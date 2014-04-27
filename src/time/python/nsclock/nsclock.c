/* @file nsclock.c
 * @author  Alexander Merritt
 * @desc: Python function to acquire nano-second-granularity clock time.
 */

#include <Python.h> // MUST be included first
#include <time.h>
#include <errno.h>

static PyObject *NSClockError;

static PyObject *
nsclock(PyObject *self, PyObject *args)
{
    /* arguments not used */
    clockid_t id = CLOCK_REALTIME;
    unsigned long long nsec;
    struct timespec sp;
    int err;

    errno = 0;
    err = clock_gettime(id, &sp);
    if (err != 0) {
        PyErr_SetFromErrno(NSClockError);
        return NULL;
    }
    nsec = sp.tv_sec * 1e9 +sp.tv_nsec;
    return Py_BuildValue("K", nsec);
}

static PyMethodDef NSClockMethods[] = {
    {"nsclock", nsclock, METH_VARARGS,
        "System time since Epoc in nanoseconds."},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initnsclock(void)
{
    PyObject *m;
    m = Py_InitModule("nsclock", NSClockMethods);
    if (m == NULL)
        return;
    NSClockError = PyErr_NewException("nsclock.err", NULL, NULL);
    Py_INCREF(NSClockError);
    PyModule_AddObject(m, "error", NSClockError);
}
