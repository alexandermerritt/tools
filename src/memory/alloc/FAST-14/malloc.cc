/*
 * Copyright (C) 2015-2016 Alexander Merritt, merritt.alex@gatech.edu
 *
 * Synthetic workload generation for testing malloc implementations
 * based on the workloads motivating this paper:
 *
 * Stephen M. Rumble, Ankita Kejriwal, and John Ousterhout. 2014.
 * Log-structured memory for DRAM-based storage. In Proceedings of the
 * 12th USENIX conference on File and Storage Technologies (FAST'14).
 * USENIX Association, Berkeley, CA, USA, 1-16.
 *
 * Specify -DWITH_REDIS if you want to include the Redis evaluation.
 * Requires a hack to the Redis sources, however.
 */

#include <fstream>
#include <random>
#include <sstream>
#include <iostream>
#include <memory>
#include <array>
#include <deque>
#include <functional>
#include <iomanip>
#include <map>

#if defined(WITH_REDIS)
#include <redox.hpp>
#endif

#include <cassert>
#include <cstddef>

#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

#if defined(WITH_REDIS)
// Make sure to configure redis with some policy to drop values if max
// memory limit has been reached, for example:
//      maxmemory-policy allkeys-random
// That way we can keep a live fixed-size working set while injecting
// large amounts of objects.

// We assume redis.conf to use is also in this directory.
// Daemonize the service. Set a memory limit, and make sure the limit
// is known to an instance of RedisSet.
#define REDIS_DIR "/opt/data/media/redis-3.0.5/src"
#define REDIS_CONF_FILE REDIS_DIR "/redis.conf"
#define REDIS_PID_FILE "/var/run/redis/redis.pid"

#define REDIS_BATCH_COUNT   1
#define REDIS_SYNC_EVERY    1
#endif  /* WITH_REDIS */

#define KB  (1UL<<10)
#define MB  (1UL<<20)
#define GB  (1UL<<30)

#define PAGE_SHIFT  12
#define PAGE_MASK   ((1UL<<PAGE_SHIFT)-1)

static inline long ceil4K(long bytes)
{
    long pgs = bytes >> PAGE_SHIFT;
    pgs += !!(bytes & PAGE_MASK);
    return (pgs << PAGE_SHIFT);
}

static inline void clear_line(void)
{
    for (int i = 0; i < 128; i++)
        printf("\b");
    fflush(stdout);
}

typedef enum {
    STAT_VMSIZE = 23,
    STAT_RSS = 24,
} statfield_t;

// getstat(STAT_VMSIZE)
// getstat(STAT_RSS, 12)        because RSS is reported in pages
long getstat(statfield_t t, size_t shift = 0UL, pid_t pid = 0)
{
    long value;
    string line;
    array<char, 512> cline;
    if (pid == 0)
        pid = getpid(); // self
    stringstream input;
    const char *val = NULL;
    input << "/proc/" << pid << "/stat";
    ifstream ifs(input.str());
    getline(ifs, line);
    memcpy(cline.data(), line.c_str(), min(line.size(),cline.size()));
    strtok(cline.data(), " ");
    for (int f = 0; f < t - 1; f++)
        val = strtok(NULL, " ");
    sscanf(val, "%ld", &value);
    return (value<<shift);
}

static inline
void *map_alloc(size_t bytes)
{
    void *v = mmap(NULL, bytes, PROT_READ|PROT_WRITE,
            MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (v == MAP_FAILED) {
        fprintf(stderr, "mmap: bytes = %lu\n", bytes);
        perror("mmap");
        return NULL;
    }
    mlock(v, bytes);
    return v;
}

class LiveSet
{
    public:
        LiveSet(long bytes)
            : max_live(bytes), live(0L),
            last_idx(0), clk(0),
            locs(nullptr), sizes(nullptr),
            fltop(0L), gen(rd()), dropRand(0, nlocs-1) {
            if (bytes < 1)
                throw runtime_error("LiveSet with empty set?");
            locs = (uintptr_t*)map_alloc(nlocs*sizeof(*locs));
            sizes = (uint32_t*)map_alloc(nlocs*sizeof(*sizes));
            assert( locs && sizes );
            freelist.fill(-1);
        }

        ~LiveSet() {
            freeAll();
            munmap(locs, nlocs*sizeof(*locs));
            munmap(sizes, nlocs*sizeof(*sizes));
        }

        // long func(void)
        typedef function<long (void)> numGen_f;

        void injectValues(numGen_f genf, long injectSz) {
            off_t idx, nn(0L), objsize, total(injectSz);
            struct timespec t1,t2;

            if (injectSz < 1)
                return;

            drop(genf()<<2);

            while (injectSz > 0) {
                if (fltop == 0) {
                    if ((idx = ++last_idx) >= nlocs)
                        throw runtime_error("Exceeded nlocs");
                } else {
                    idx = freelist[--fltop];
                    assert( idx > -1 );
                }

                try { objsize = genf(); }
                // throw this in your lambda to prematurely exit
                catch (out_of_range &e) { break; }
                assert( !locs[idx] );

                clock_gettime(CLOCK_MONOTONIC, &t1);
                assert( locs[idx] = (uintptr_t)malloc(objsize) );
                clock_gettime(CLOCK_MONOTONIC, &t2);
                clk += (t2.tv_sec *1e9 - t2.tv_nsec)
                    - (t1.tv_sec *1e9 - t1.tv_nsec);

                sizes[idx] = objsize;
                live += objsize;
                injectSz -= objsize;
                nobjs++;

                drop(objsize<<2);

                // print progress
                if (!(nn++%10000L)) {
                    clear_line(); printf("# %3.3lf %%  ",
                        100.*(float)(total-injectSz)/total);
                    fflush(stdout);
                }
            }
            printf("\n");
        }

        // free objects at random until we are below threshold
        void drop(off_t atleast = 0UL) {
            if (live <= (max_live - atleast))
                return;
            long idx;
            while (live > (max_live - atleast)) {
                if (last_idx <= 0)
                    throw runtime_error("is max_live too small?");
                idx = dropRand(gen) % last_idx;
                if (!locs[idx])
                    continue;
                free((void*)locs[idx]);
                live -= sizes[idx];
                if (fltop >= flmax)
                    throw runtime_error("Exceeded freelist");
                freelist[fltop++] = idx;
                nobjs--;
                locs[idx] = sizes[idx] = 0;
            }
        }

        void freeAll() {
            for (long n = 0; n < nlocs; n++) {
                if (locs[n]) {
                    free((void*)locs[n]);
                    locs[n] = sizes[n] = 0;
                }
            }
            freelist.fill(-1);
            live = 0L;
            nobjs = 0L;
            fltop = 0L;
            last_idx = 0L;
        }

        size_t overhead() {
            return ceil4K(nlocs*sizeof(*locs))
                + ceil4K(nlocs*sizeof(*sizes))
                + ceil4K(sizeof(LiveSet));
        }

        // Class variables

        long max_live, live;    // working set max and current, bytes
        long nobjs;             // pointers allocated
        long last_idx;          // bound in locs for locating pointers
        long clk;

        // quantity of pointers we track
        constexpr static long nlocs = ((long)200e6);
        uintptr_t *locs; // addresses of allocated objects
        uint32_t *sizes; // sizes of allocated objects

    private:
        constexpr static off_t flmax = nlocs;
        array<off_t, flmax> freelist; // indexes of free slots in locs
        off_t fltop;

        random_device rd;
        mt19937 gen;
        uniform_int_distribution<long> dropRand;

        LiveSet();
};

#if defined(WITH_REDIS)
class RedisSet
{
    public:
        size_t clk; // cumulative time spent writing values (nanosec)
        pid_t pid;
        long livewss; // scraped from redis.conf automatically
        string redis_dir, redis_srv, redis_cmd;
        long initMemSize;

        // Next key to use (converted to string).
        // Values never reused.
        size_t ikey = 0;

        RedisSet(long klen = 32)
            : clk(0), pid(-1), livewss(0),
            redis_dir(REDIS_DIR),
            redis_srv("redis-server"),
            redis_cmd(redis_dir + "/" + redis_srv),
            initMemSize(0),
            keylen(klen)
        {
            stopServer(true);
            readConfig();
            startServer();
            connect();
            // Subtract this out of the "wasted" memory calculations.
            // It includes the library mappings, code, globals, etc.
            initMemSize = getVMSize();
            // Just some ad-hoc sanity check, make sure redis isn't
            // mmap'ing enormous buffers in advance.  If it does, one
            // cause might be the specific malloc library
            // implementation it links with.
            if (initMemSize > (64L<<20))
                throw runtime_error("Redis init VM size > 64MB (why?)");
        }

        ~RedisSet() {
            disconnect();
            stopServer(true);
        }

        long getKeyLen(void) { return keylen; }
        void setKeyLen(long klen) { keylen = klen; }

        void injectValues(LiveSet::numGen_f genf, long injectSz) {
            struct timespec t1,t2;
            long len, total = injectSz, nn = 0;
            ostringstream key;
            vector<string> cmd;
            bool done = false;

            const long batch_amt = REDIS_BATCH_COUNT;
            while (injectSz > 0 && !done) {
                cmd.clear();
                cmd.push_back("MSET");
                size_t batch_len = 0;
                for (int ii = 0; ii < batch_amt; ii++, nn++) {
                    try { len = genf(); }
                    // throw this in your lambda to prematurely exit
                    catch (out_of_range &e) { done=true; break; }
                    key.str(string());
                    key << setfill('0') << setw(keylen) << ikey++;
                    string value(len, '0');
                    cmd.push_back(key.str());
                    cmd.push_back(value);
                    injectSz -= len;
                    if (injectSz < 0)
                        break;
                    batch_len += len;
                    if (batch_len > (1*MB))
                        break;
                }
                if (cmd.empty())
                    break;

                // enqueue some commands asynchronously
                static size_t flip = 0;
                if (!(flip++ % REDIS_SYNC_EVERY))
                    redis.commandSync(cmd);
                else
                    redis.command(cmd);
#if 0
                clock_gettime(CLOCK_MONOTONIC, &t1);
                if (!redis.set(key.str(), value))
                    throw runtime_error("Error writing value to Redis");
                clock_gettime(CLOCK_MONOTONIC, &t2);
                clk += (t2.tv_sec * 1e9 + t2.tv_nsec)
                    - (t1.tv_sec * 1e9 + t1.tv_nsec);
                injectSz -= len;
                nn++;
#endif

                // print progress
                if (!(nn%1L)) {
                    clear_line(); printf("# %3.3lf %%  ",
                        100.*(float)(total-injectSz)/total);
                    fflush(stdout);
                }
            }
            printf("\n");
        }

        // drop(0.9) removes 90% of current capacity by reducing the
        // maxmemory config parameter temporarily
        void drop(float drop_pct) {
            long dropmem = (long)(livewss * (1.-drop_pct));
            vector<string> cmd;
            cmd.push_back("CONFIG");
            cmd.push_back("SET");
            cmd.push_back("maxmemory");

            printf("# dropping from %.2f to %.2f MiB\n",
                    (float)livewss/MB, (float)dropmem/MB);

            cmd.push_back(to_string(dropmem));
            redis.commandSync(cmd);

            // trigger release of memory by inserting key
            string key("delete me soon please"), &value(key);
            redis.set(key, value);
            redis.del(key);

            cmd.pop_back();
            cmd.push_back(to_string(livewss));
            redis.commandSync(cmd);
        }

        pid_t readPID(string pidfile = REDIS_PID_FILE) {
            if (pid > -1)
                return pid;
            ifstream ifs(pidfile);
            if (!ifs.is_open())
                throw runtime_error("Cannot open redis pid file");
            ifs >> pid;
            ifs.close();
            return pid;
        }

        float getVMSize(void) {
            if (pid < 0)
                readPID();
            return (float)getstat(STAT_VMSIZE, 0, pid);
        }

        // Expect the response something like this:
        //      # Keyspace
        // for no keys, or for > 0 keys:
        //      # Keyspace
        //      dbN:keys=52,expires=0,avg_ttl=0,keysize=0,objsize=0
        //      [...]
        // where N is some integer. keysize and objsize were added by
        // me in a hacked patch to redis, and aren't part of the
        // standard Redis implementation.
        map<string,long> * getInfoKeyspace(void) {
            map<string,long> *m = nullptr;
            string line;
            vector<string> cmd;
            stringstream ss;
            size_t loc;

            assert( (m = new map<string,long>()) );

            cmd.push_back("info");
            cmd.push_back("keyspace");
            auto &cmdObj = redis.commandSync<string>(cmd);
            ss = stringstream(cmdObj.reply());
            cmdObj.free();

            while (getline(ss, line)) {
                if (line.empty())
                    continue;
                if (line.find("db") != 0)
                    continue;
                // all information in one line
                for (int i = 0; i < 3; i++) {
                    string s;
                    switch (i) {
                        case 0: { s = "keys"; }; break;
                        case 1: { s = "keysize"; }; break;
                        case 2: { s = "objsize"; }; break;
                    }
                    if (string::npos == (loc = line.find(s)))
                        throw runtime_error("Unexpected Redis keyspace line");
                    string chop = line.substr(loc + s.size() + 1); // +1 for =
                    long v = strtoll(chop.c_str(), NULL, 10);
                    pair<string,long> p(s,v);
                    m->insert(p);
                }
            }

            return m;
        }

    private:
        redox::Redox redis;
        size_t keylen;

        void startServer(void)
        {
            char cmd[256];
            int b = snprintf(cmd, 255, "%s %s",
                    redis_cmd.c_str(), REDIS_CONF_FILE);
            cmd[b] = '\0';
            if (system(cmd))
                throw runtime_error("Error starting redis");
            sleep(1); // give time to write PID file
            readPID();
        }

        void stopServer(bool ignore = false)
        {
            char cmd[64];
            int b = snprintf(cmd, 63, "killall -q %s", redis_srv.c_str());
            cmd[b] = '\0';
            if (system(cmd) && !ignore)
                throw runtime_error("Error stopping redis");
            sleep(1);
        }

        void connect(void) {
            if (!redis.connectUnix())
                throw runtime_error("Cannot connect to redis");
        }

        void disconnect(void) {
            redis.disconnect();
        }

        void readConfig(string conffile = REDIS_CONF_FILE) {
            ifstream ifs(conffile);
            if (!ifs.is_open())
                throw runtime_error("Error opening conf file");
            string line;
            while (getline(ifs, line)) {
                if (line.empty())
                    continue;
                if (line.at(0) == '#')
                    continue;
                // split line
                string key, value;
                stringstream ss;
                ss << line;
                ss >> key;
                // extract memory parameter
                if (key == "maxmemory") {
                    ss >> value;
                    livewss = strtoll(value.c_str(), NULL, 10);
                    size_t i = value.find_first_of("kKmMgG");
                    if (i == string::npos)
                        return; // unit is bytes
                    string unit(value.substr(i));
                    for (size_t c = 0; c < unit.size(); c++)
                        unit[c] = tolower(unit[c]);
                    long factor = 0;
                    if (unit == "k") { factor = (long)1e3; }
                    else if (unit == "m") { factor = (long)1e6; }
                    else if (unit == "g") { factor = (long)1e9; }
                    else if (unit == "kb") { factor = KB; }
                    else if (unit == "mb") { factor = MB; }
                    else if (unit == "gb") { factor = GB; }
                    else {
                        throw runtime_error("Cannot determine "
                                "Redis memory limit unit from conf");
                    }
                    livewss *= factor;
                    break; // skip remainder of config
                }
            }
            ifs.close();
        }
};
#endif  /* WITH_REDIS */

// Generator for the number lambda funcs to use.
static random_device rd;
static mt19937 gen(rd());

//
// Modeled after synthetic workload from: Rumble et al. FAST'14
//
auto W1before = [] () { return 100L; };
auto W2before = W1before;
auto W3before = W1before;
auto W4before = [] () {
    const long m1 = 100, m2 = 150;
    static uniform_int_distribution<long> d(m1, m2);
    return d(gen);
};
auto W5before = W4before;
auto W6before = [] () {
    const long m1 = 100, m2 = 250;
    static uniform_int_distribution<long> d(m1, m2);
    return d(gen);
};
auto W7before = [] () {
    const long m1 = 1000, m2 = 2000;
    static uniform_int_distribution<long> d(m1, m2);
    return d(gen);
};
auto W8before = [] () {
    const long m1 = 50, m2 = 150;
    static uniform_int_distribution<long> d(m1, m2);
    return d(gen);
};

auto W2after  = [] () { return 130L; };
auto W3after  = W2after;
auto W4after  = [] () {
    const long m1 = 200, m2 = 250;
    static uniform_int_distribution<long> d(m1, m2);
    return d(gen);
};
auto W5after  = W4after;
auto W6after  = W7before;
auto W7after  = [] () {
    const long m1 = 1500, m2 = 2500;
    static uniform_int_distribution<long> d(m1, m2);
    return d(gen);
};
auto W8after  = [] () {
    const long m1 = 5000, m2 = 15000;
    static uniform_int_distribution<long> d(m1, m2);
    return d(gen);
};

// addl is any additional memory usage we wish to subtract (e.g. which
// may be used for tracking or expmt setup outside of liveset)
void dumpstatsL(const char *prog, const char *test,
        long injectwss, LiveSet *liveset, long addl = 0)
{
    float mem = (float)getstat(STAT_VMSIZE);
    float wss = mem - liveset->overhead() - addl;
    float eff = wss/liveset->live;
    printf("prog cmd live inject mem wss eff LSoverhead nobjs clk_ms\n");
    printf("%s %s %.2lf %.2lf %.2lf %.2lf %.4lf %.2lf %ld %.3lf\n",
            prog, test,
            (float)liveset->live/MB,
            (float)injectwss/MB,
            mem/MB, wss/MB, eff,
            (float)liveset->overhead()/MB,
            liveset->nobjs, liveset->clk/1e6);
}

#if defined(WITH_REDIS)
void dumpstats(const char *test,
        long injectwss, RedisSet *redis)
{
    map<string,long> *info = redis->getInfoKeyspace();
    // don't subtract the initial vmsize as the code, stack,
    // libraries, etc. all do count against memory efficiency..
    float mem = (float)redis->getVMSize(); // - redis->initMemSize;
    float wss = info->at("keysize") + info->at("objsize");
    // NOTE: with redis.conf configured for a maximum memory limit, we
    // calculate efficiency opposite to something which does not have
    // a limit, namely, ONCE FULL and redis has begun to evict keys to
    // accomodate new ones, the vmsize will be greater than the sum of
    // keys + objects combined
    float eff = mem/wss;
    // live is maxmemory configured in redis.conf
    printf("prog cmd live inject mem wss eff nobjs\n");
    printf("redis %s %.2lf %.2lf %.2lf %.2lf %.4lf %ld\n",
            test, (float)redis->livewss/MB, (float)injectwss/MB,
            mem/MB, wss/MB, eff, info->at("keys"));
    delete info;
}
#endif  /* WITH_REDIS */

void sizes_on_stdin(deque<long> &values)
{
    string line;
    long s;
    while (cin >> line) {
        s = strtoll(line.c_str(), NULL, 10);
        values.push_back(s);
    }
}

#if defined(WITH_REDIS)
// we can carelessly allocate memory in this process when evaluting
// redis, because redis is in another process (and we are not
// measuring self)
int doredis(int narg, char *args[])
{
    RedisSet *redis;
    long keymem, injectwss;

    if (narg != 3) {
        cerr << "Usage: " << *args
            << " cmd inject_wss"
            << endl << "\t(wss specified in MiB)"
            << endl << "\t(max mem limit and behavior configured in redis.conf)"
            << endl;
        exit(1);
    }

    redis = new RedisSet();
    assert( redis );

#if 0
    cout << "# redis maxmemory: "
        << redis->livewss / MB << " MiB"
        << " vmsize: "
        << getstat(STAT_VMSIZE, 0, redis->pid) / MB << " MiB"
        << " rss: "
        << getstat(STAT_RSS, 12, redis->pid) / MB << " MiB"
        << endl;
#endif

    string cmd(args[1]);
    injectwss = strtoll(args[2], NULL, 10);

    if (cmd == "w1") {
        redis->injectValues(W1before, injectwss);
    } else if (cmd == "w2") {
        redis->injectValues(W2before, injectwss);
        redis->injectValues(W2after, injectwss);
    } else if (cmd == "w3") {
        redis->injectValues(W3before, injectwss);
        redis->drop(0.9);
        redis->injectValues(W3after, injectwss);
    } else if (cmd == "w4") {
        redis->injectValues(W4before, injectwss);
        redis->injectValues(W4after, injectwss);
    } else if (cmd == "w5") {
        redis->injectValues(W5before, injectwss);
        redis->drop(0.9);
        redis->injectValues(W5after, injectwss);
    } else if (cmd == "w6") {
        redis->injectValues(W6before, injectwss);
        redis->drop(0.5);
        redis->injectValues(W6after, injectwss);
    } else if (cmd == "w7") {
        redis->injectValues(W7before, injectwss);
        redis->drop(0.9);
        redis->injectValues(W7after, injectwss);
    } else if (cmd == "w8") {
        redis->injectValues(W8before, injectwss);
        redis->drop(0.9);
        redis->injectValues(W8after, injectwss);
    } else if (cmd == "stdin") {
        static deque<long> values;
        sizes_on_stdin(values);
        LiveSet::numGen_f vfn = [&] () -> long {
            if (values.empty()) throw out_of_range("");
            long v = values.front();
            values.pop_front();
            return v;
        };
        redis->injectValues(vfn, injectwss);
    } else if (cmd == "image") {
        LiveSet::numGen_f vfn = [] () -> long {
            //static normal_distribution<long> sm(25*KB, 500*KB);
            static uniform_int_distribution<long> sm(25*KB, 500*KB);
            //static uniform_int_distribution<long> lg(250*KB, 5*MB);
            //static size_t o = 0UL;
            //if ((o++ % 100) == 0)
                //return lg(gen);
            //else
                //return sm(gen);
            return sm(gen);
        };
        redis->injectValues(vfn, injectwss);
    } else {
        cerr << "Unknown workload to run." << endl;
        exit(1);
    }

    dumpstats(args[1], injectwss, redis);

    delete redis;
    return 0;
}
#endif  /* WITH_REDIS */

int main(int narg, char *args[])
{
    string name(*args);

#if defined(WITH_REDIS)
    if (name == "./redistest")
        return doredis(narg, args);
#endif

    if (narg != 4) {
        cerr << "Usage: " << *args
            << " command live_wss inject_wss"
            << endl << "\t(wss specified in bytes)"
            << endl;
        exit(1);
    }

    string cmd(args[1]);
    long livewss = strtoll(args[2], NULL, 10);
    long injectwss = strtoll(args[3], NULL, 10);

    // Must allocate liveset on heap because freelist array is huge.
    // Additionally, do this outside of the purview of malloc.
    LiveSet *liveset(nullptr);

    liveset = new (map_alloc(sizeof(LiveSet))) LiveSet(livewss);
    assert( liveset );
    long addl = 0L;

    if (cmd == "w1") {
        liveset->injectValues(W1before, injectwss);
    } else if (cmd == "w2") {
        liveset->injectValues(W2before, injectwss);
        liveset->injectValues(W2after, injectwss);
    } else if (cmd == "w3") {
        liveset->injectValues(W3before, injectwss);
        liveset->drop(livewss * 0.9);
        liveset->injectValues(W3after, injectwss);
    } else if (cmd == "w4") {
        liveset->injectValues(W4before, injectwss);
        liveset->injectValues(W4after, injectwss);
    } else if (cmd == "w5") {
        liveset->injectValues(W5before, injectwss);
        liveset->drop(livewss * 0.9);
        liveset->injectValues(W5after, injectwss);
    } else if (cmd == "w6") {
        liveset->injectValues(W6before, injectwss);
        liveset->drop(livewss * 0.5);
        liveset->injectValues(W6after, injectwss);
    } else if (cmd == "w7") {
        liveset->injectValues(W7before, injectwss);
        liveset->drop(livewss * 0.9);
        liveset->injectValues(W7after, injectwss);
    } else if (cmd == "w8") {
        liveset->injectValues(W8before, injectwss);
        liveset->drop(livewss * 0.9);
        liveset->injectValues(W8after, injectwss);
    } else if (cmd == "stdin") {
        // FIXME Some allocators preallocate memory during library
        // initialization. Subtracting vmsize at this point would
        // include this amount (incorrect measurements).
        //float mem = (float)getstat(STAT_VMSIZE);
        //addl += ((float)getstat(STAT_VMSIZE) - mem);
        LiveSet::numGen_f vfn = [] () -> long {
            string line;
            long v;
            if (!cin.eof() && cin >> line) {
                v = strtoll(line.c_str(), NULL, 10);
            } else {
                throw out_of_range("no more stdin");
            }
            return v;
        };
        liveset->injectValues(vfn, injectwss);
    } else if (cmd == "img") {
        LiveSet::numGen_f vfn1 = [] () -> long {
            const long m1 = 25*KB, m2 = 500*KB;
            static uniform_int_distribution<long> d(m1, m2);
            return d(gen);
        };
        LiveSet::numGen_f vfn2 = [] () -> long {
            const long m1 = 10*KB, m2 = 5000*KB;
            static uniform_int_distribution<long> d(m1, m2);
            return d(gen);
        };
        LiveSet::numGen_f vfn3 = [] () -> long {
            static uniform_int_distribution<long> sm(25*KB, 500*KB);
            static uniform_int_distribution<long> lg(250*KB, 5*MB);
            static size_t o = 0UL;
            if ((o++ % 100) == 0)
                return lg(gen);
            else
                return sm(gen);
        };
        //liveset->injectValues(vfn1, injectwss);
        //liveset->injectValues(vfn2, injectwss);

        // http://vis-www.cs.umass.edu/lfw/
        LiveSet::numGen_f LFW = [] () -> long {
            const float lfw_min = 7648, lfw_max = 26450;
            const float lfw_mean = 14234, lfw_sd = 2271;
            static normal_distribution<float> d(lfw_mean, lfw_sd);
            long v = static_cast<long>(d(gen));
            return v;
        };
        // decoded JPEG from LFW (all have same sizes)
        LiveSet::numGen_f LFW2 = [] () -> long {
            return 187500L;
        };
        //liveset->injectValues(LFW, injectwss);
        //liveset->injectValues(LFW2, injectwss);

        LiveSet::numGen_f LFWx = [] () -> long {
            const float lfw_mean = 14234, lfw_sd = 2271;
            static normal_distribution<float> d(lfw_mean, lfw_sd);
            static uniform_real_distribution<float> g(1.1, 0.2);
            static long i = 0L;
            switch (i%2) {
                case 0:
                    return d(gen); // the image
                default:
                    return d(gen) * (187500L * g(gen)); // the analytics
            }
        };
        liveset->injectValues(LFWx, injectwss);
    } else {
        cerr << "Unknown workload to run." << endl;
        exit(1);
    }
    dumpstatsL(*args, args[1], injectwss, liveset, addl);
    return 0;
}

