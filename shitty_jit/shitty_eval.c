#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <frameobject.h>

ssize_t code_object_index = -1;
static struct PyModuleDef _shitty_eval;

// eval_custom_code
// Evaluate a frame with a custom code object
inline static PyObject *eval_custom_code(PyThreadState *tstate, PyFrameObject *frame,
                                         PyCodeObject *code, int throw_flag) {
  Py_ssize_t ncells = 0;
  Py_ssize_t nfrees = 0;
  Py_ssize_t nlocals_new = code->co_nlocals;
  Py_ssize_t nlocals_old = frame->f_code->co_nlocals;
  if ((code->co_flags & CO_NOFREE) == 0) {
    ncells = PyTuple_GET_SIZE(code->co_cellvars);
    nfrees = PyTuple_GET_SIZE(code->co_freevars);
  }
  PyFrameObject *shadow = PyFrame_New(tstate, code, frame->f_globals, NULL);
  if (shadow == NULL) {
    return NULL;
  }
  PyObject **fastlocals_old = frame->f_localsplus;
  PyObject **fastlocals_new = shadow->f_localsplus;
  for (Py_ssize_t i = 0; i < nlocals_old; i++) {
    Py_XINCREF(fastlocals_old[i]);
    fastlocals_new[i] = fastlocals_old[i];
  }
  for (Py_ssize_t i = 0; i < ncells + nfrees; i++) {
    Py_XINCREF(fastlocals_old[nlocals_old + i]);
    fastlocals_new[nlocals_new + i] = fastlocals_old[nlocals_old + i];
  }
  PyObject *result = _PyEval_EvalFrameDefault(tstate, shadow, throw_flag);
  Py_DECREF(shadow);
  return result;
}

// eval_frame_c
// The function we will have the interpreter call to evaluate a frame
static PyObject *eval_frame_c(PyThreadState *tstate, PyFrameObject *frame, int throw_flag) {
  // These are too scary to handle :-(
  if (frame->f_code->co_flags & (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR)) {
    return _PyEval_EvalFrameDefault(tstate, frame, throw_flag);
  }

  // Check if we have already created a replacement code object for this frame
  // We're using the PEP 523 APIs to stash it in co_extra
  PyObject *ef_code;
  _PyCode_GetExtra((PyObject *)frame->f_code, code_object_index, (void **)&ef_code);

  // If we haven't, create our replacement code object
  if (ef_code == NULL) {
    // Set the frame evaluator to the default, so that _eval_frame can call into Python
    _PyInterpreterState_SetEvalFrameFunc(tstate->interp, &_PyEval_EvalFrameDefault);

    // Delegate to our (potentially pure Python) _eval_frame function
    PyObject *module = PyState_FindModule(&_shitty_eval);
    PyObject *ef = PyObject_GetAttrString(module, "_eval_frame");
    ef_code = PyObject_CallOneArg(ef, (PyObject *)frame);
    Py_DECREF(ef);

    _PyInterpreterState_SetEvalFrameFunc(tstate->interp, &eval_frame_c);

    if (ef_code && !PyCode_Check(ef_code)) {
      PyErr_SetString(PyExc_TypeError, "expected a code object");
      return NULL;
    }

    // Stash it for future use!
    _PyCode_SetExtra((PyObject *)frame->f_code, code_object_index, ef_code);
  }

  // And finally, evaluate the frame but with our code object
  return eval_custom_code(tstate, frame, (PyCodeObject *)ef_code, throw_flag);
  // return _PyEval_EvalFrameDefault(tstate, frame, throw_flag);
}

// set_eval_frame_func
// First, we set _shitty_eval._eval_frame to the given function
// Then, we use the PEP 523 API to set the frame evaluator to the eval_frame_c function above
// If the given function is None, we reset the frame evaluator to the default
static PyObject *set_eval_frame_func(PyObject *dummy, PyObject *args) {
  PyObject *new_ef = NULL;
  if (!PyArg_ParseTuple(args, "O:eval_frame_func", &new_ef)) {
    return NULL;
  }
  if (new_ef != Py_None && !PyCallable_Check(new_ef)) {
    PyErr_SetString(PyExc_TypeError, "expected a callable or None");
    return NULL;
  }

  // Set _shitty_eval._eval_frame to the given function
  PyThreadState *tstate = PyThreadState_GET();
  PyObject *module = PyState_FindModule(&_shitty_eval);
  PyObject *old_ef = PyObject_GetAttrString(module, "_eval_frame");
  PyObject_SetAttrString(module, "_eval_frame", new_ef);

  // Set the frame evaluator to eval_frame_c or reset it to the default
  if (old_ef != Py_None && new_ef == Py_None) {
    _PyInterpreterState_SetEvalFrameFunc(tstate->interp, &_PyEval_EvalFrameDefault);
  } else if (old_ef == Py_None && new_ef != Py_None) {
    _PyInterpreterState_SetEvalFrameFunc(tstate->interp, &eval_frame_c);
  }
  Py_DECREF(old_ef);

  Py_RETURN_NONE;
}

// Module definitions

static PyMethodDef _methods[] = {{"set_eval_frame_func", set_eval_frame_func, METH_VARARGS, NULL},
                                 {NULL, NULL, 0, NULL}};

static struct PyModuleDef _shitty_eval = {PyModuleDef_HEAD_INIT, "_shitty_eval", NULL, -1,
                                          _methods};

PyMODINIT_FUNC PyInit__shitty_eval(void) {
  PyObject *module = PyModule_Create(&_shitty_eval);
  if (module == NULL) {
    return NULL;
  }

  PyObject *eval_frame = Py_None;
  Py_INCREF(eval_frame);
  if (PyModule_AddObject(module, "_eval_frame", eval_frame) < 0) {
    Py_DECREF(module);
    Py_DECREF(eval_frame);
    return NULL;
  }

  code_object_index = _PyEval_RequestCodeExtraIndex(NULL);
  if (code_object_index == -1) {
    Py_DECREF(module);
    return NULL;
  }

  return module;
}
