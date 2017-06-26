/*
 * pythonmod.c: unbound module C wrapper
 *
 * Copyright (c) 2009, Zdenek Vasicek (vasicek AT fit.vutbr.cz)
 *                     Marek Vavrusa  (xvavru00 AT stud.fit.vutbr.cz)
 *
 * This software is open source.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    * Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *    * Neither the name of the organization nor the names of its
 *      contributors may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * \file
 * Python module for unbound.  Calls python script.
 */

/* ignore the varargs unused warning from SWIGs internal vararg support */
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif

#include "config.h"
#include "sldns/sbuffer.h"

#undef _POSIX_C_SOURCE
#undef _XOPEN_SOURCE
#include <Python.h>

#include "pythonmod/pythonmod.h"
#include "util/module.h"
#include "util/config_file.h"
#include "pythonmod_utils.h"

#ifdef S_SPLINT_S
typedef struct PyObject PyObject;
typedef struct PyThreadState PyThreadState;
typedef void* PyGILState_STATE;
#endif

/**
 * Global state for the module.
 */
struct pythonmod_env {

	/** Python script filename. */
	const char* fname;

	/** Python main thread */
	PyThreadState* mainthr;
	/** Python module. */
	PyObject* module;

	/** Module init function */
	PyObject* func_init;
	/** Module deinit function */
	PyObject* func_deinit;
	/** Module operate function */
	PyObject* func_operate;
	/** Module super_inform function */
	PyObject* func_inform;

	/** Python dictionary. */
	PyObject* dict;

	/** Module data. */
	PyObject* data;

	/** Module qstate. */
	struct module_qstate* qstate;
};

/**
 * Per query state for the iterator module.
 */
struct pythonmod_qstate {

	/** Module per query data. */
	PyObject* data;
};

/* Generated */
#ifndef S_SPLINT_S
#include "pythonmod/interface.h"
#endif

int pythonmod_init(struct module_env* env, int id)
{
   /* Initialize module */
   FILE* script_py = NULL;
   PyObject* py_init_arg, *res;
   PyGILState_STATE gil;
   int init_standard = 1;

   struct pythonmod_env* pe = (struct pythonmod_env*)calloc(1, sizeof(struct pythonmod_env));
   if (!pe)
   {
      log_err("pythonmod: malloc failure");
      return 0;
   }

   env->modinfo[id] = (void*) pe;

   /* Initialize module */
   pe->fname = env->cfg->python_script;
   if(pe->fname==NULL || pe->fname[0]==0) {
      log_err("pythonmod: no script given.");
      return 0;
   }

   /* Initialize Python libraries */
   if (!Py_IsInitialized())
   {
#if PY_MAJOR_VERSION >= 3
      wchar_t progname[8];
      mbstowcs(progname, "unbound", 8);
#else
      char *progname = "unbound";
#endif
      Py_SetProgramName(progname);
      Py_NoSiteFlag = 1;
#if PY_MAJOR_VERSION >= 3
      PyImport_AppendInittab(SWIG_name, (void*)SWIG_init);
#endif
      Py_Initialize();
      PyEval_InitThreads();
      SWIG_init();
      pe->mainthr = PyEval_SaveThread();
   }

   gil = PyGILState_Ensure();

   /* Initialize Python */
   PyRun_SimpleString("import sys \n");
   PyRun_SimpleString("sys.path.append('.') \n");
   if(env->cfg->directory && env->cfg->directory[0]) {
      char wdir[1524];
      snprintf(wdir, sizeof(wdir), "sys.path.append('%s') \n",
      env->cfg->directory);
      PyRun_SimpleString(wdir);
   }
   PyRun_SimpleString("sys.path.append('"RUN_DIR"') \n");
   PyRun_SimpleString("sys.path.append('"SHARE_DIR"') \n");
   PyRun_SimpleString("import distutils.sysconfig \n");
   PyRun_SimpleString("sys.path.append(distutils.sysconfig.get_python_lib(1,0)) \n");
   if (PyRun_SimpleString("from unboundmodule import *\n") < 0)
   {
      log_err("pythonmod: cannot initialize core module: unboundmodule.py");
      PyGILState_Release(gil);
      return 0;
   }

   /* Check Python file load */
   if ((script_py = fopen(pe->fname, "r")) == NULL)
   {
      log_err("pythonmod: can't open file %s for reading", pe->fname);
      PyGILState_Release(gil);
      return 0;
   }

   /* Load file */
   pe->module = PyImport_AddModule("__main__");
   pe->dict = PyModule_GetDict(pe->module);
   pe->data = Py_None;
   Py_INCREF(pe->data);
   PyModule_AddObject(pe->module, "mod_env", pe->data);

   /* TODO: deallocation of pe->... if an error occurs */

   if (PyRun_SimpleFile(script_py, pe->fname) < 0)
   {
      log_err("pythonmod: can't parse Python script %s", pe->fname);
      PyGILState_Release(gil);
      return 0;
   }

   fclose(script_py);

   if ((pe->func_init = PyDict_GetItemString(pe->dict, "init_standard")) == NULL)
   {
      init_standard = 0;
      if ((pe->func_init = PyDict_GetItemString(pe->dict, "init")) == NULL)
      {
         log_err("pythonmod: function init is missing in %s", pe->fname);
         PyGILState_Release(gil);
         return 0;
      }
   }
   if ((pe->func_deinit = PyDict_GetItemString(pe->dict, "deinit")) == NULL)
   {
      log_err("pythonmod: function deinit is missing in %s", pe->fname);
      PyGILState_Release(gil);
      return 0;
   }
   if ((pe->func_operate = PyDict_GetItemString(pe->dict, "operate")) == NULL)
   {
      log_err("pythonmod: function operate is missing in %s", pe->fname);
      PyGILState_Release(gil);
      return 0;
   }
   if ((pe->func_inform = PyDict_GetItemString(pe->dict, "inform_super")) == NULL)
   {
      log_err("pythonmod: function inform_super is missing in %s", pe->fname);
      PyGILState_Release(gil);
      return 0;
   }

   if (init_standard)
   {
      py_init_arg = SWIG_NewPointerObj((void*) env, SWIGTYPE_p_module_env, 0);
   }
   else
   {
      py_init_arg = SWIG_NewPointerObj((void*) env->cfg,
        SWIGTYPE_p_config_file, 0);
   }
   res = PyObject_CallFunction(pe->func_init, "iO", id, py_init_arg);
   if (PyErr_Occurred())
   {
      log_err("pythonmod: Exception occurred in function init");
      PyErr_Print();
      Py_XDECREF(res);
      Py_XDECREF(py_init_arg);
      PyGILState_Release(gil);
      return 0;
   }

   Py_XDECREF(res);
   Py_XDECREF(py_init_arg);
   PyGILState_Release(gil);

   return 1;
}

void pythonmod_deinit(struct module_env* env, int id)
{
   struct pythonmod_env* pe = env->modinfo[id];
   if(pe == NULL)
      return;

   /* Free Python resources */
   if(pe->module != NULL)
   {
      PyObject* res;
      PyGILState_STATE gil = PyGILState_Ensure();

      /* Deinit module */
      res = PyObject_CallFunction(pe->func_deinit, "i", id);
      if (PyErr_Occurred()) {
         log_err("pythonmod: Exception occurred in function deinit");
         PyErr_Print();
      }
      /* Free result if any */
      Py_XDECREF(res);
      /* Free shared data if any */
      Py_XDECREF(pe->data);
      PyGILState_Release(gil);

      PyEval_RestoreThread(pe->mainthr);
      Py_Finalize();
      pe->mainthr = NULL;
   }
   pe->fname = NULL;
   free(pe);

   /* Module is deallocated in Python */
   env->modinfo[id] = NULL;
}

void pythonmod_inform_super(struct module_qstate* qstate, int id, struct module_qstate* super)
{
   struct pythonmod_env* pe = (struct pythonmod_env*)qstate->env->modinfo[id];
   struct pythonmod_qstate* pq = (struct pythonmod_qstate*)qstate->minfo[id];
   PyObject* py_qstate, *py_sqstate, *res;
   PyGILState_STATE gil = PyGILState_Ensure();

   log_query_info(VERB_ALGO, "pythonmod: inform_super, sub is", &qstate->qinfo);
   log_query_info(VERB_ALGO, "super is", &super->qinfo);

   py_qstate = SWIG_NewPointerObj((void*) qstate, SWIGTYPE_p_module_qstate, 0);
   py_sqstate = SWIG_NewPointerObj((void*) super, SWIGTYPE_p_module_qstate, 0);

   res = PyObject_CallFunction(pe->func_inform, "iOOO", id, py_qstate,
	py_sqstate, pq->data);

   if (PyErr_Occurred())
   {
      log_err("pythonmod: Exception occurred in function inform_super");
      PyErr_Print();
      qstate->ext_state[id] = module_error;
   }
   else if ((res == NULL)  || (!PyObject_IsTrue(res)))
   {
      log_err("pythonmod: python returned bad code in inform_super");
      qstate->ext_state[id] = module_error;
   }

   Py_XDECREF(res);
   Py_XDECREF(py_sqstate);
   Py_XDECREF(py_qstate);

   PyGILState_Release(gil);
}

void pythonmod_operate(struct module_qstate* qstate, enum module_ev event,
	int id, struct outbound_entry* ATTR_UNUSED(outbound))
{
   struct pythonmod_env* pe = (struct pythonmod_env*)qstate->env->modinfo[id];
   struct pythonmod_qstate* pq = (struct pythonmod_qstate*)qstate->minfo[id];
   PyObject* py_qstate, *res;
   PyGILState_STATE gil = PyGILState_Ensure();

   if ( pq == NULL)
   {
      /* create qstate */
      pq = qstate->minfo[id] = malloc(sizeof(struct pythonmod_qstate));

      /* Initialize per query data */
      pq->data = Py_None;
      Py_INCREF(pq->data);
   }

   /* Call operate */
   py_qstate = SWIG_NewPointerObj((void*) qstate, SWIGTYPE_p_module_qstate, 0);
   res = PyObject_CallFunction(pe->func_operate, "iiOO", id, (int) event,
	py_qstate, pq->data);
   if (PyErr_Occurred())
   {
      log_err("pythonmod: Exception occurred in function operate, event: %s", strmodulevent(event));
      PyErr_Print();
      qstate->ext_state[id] = module_error;
   }
   else if ((res == NULL)  || (!PyObject_IsTrue(res)))
   {
      log_err("pythonmod: python returned bad code, event: %s", strmodulevent(event));
      qstate->ext_state[id] = module_error;
   }
   Py_XDECREF(res);
   Py_XDECREF(py_qstate);

   PyGILState_Release(gil);
}

void pythonmod_clear(struct module_qstate* qstate, int id)
{
   struct pythonmod_qstate* pq;
   if (qstate == NULL)
      return;

   pq = (struct pythonmod_qstate*)qstate->minfo[id];
   verbose(VERB_ALGO, "pythonmod: clear, id: %d, pq:%lX", id,
   	(unsigned long int)pq);
   if(pq != NULL)
   {
      PyGILState_STATE gil = PyGILState_Ensure();
      Py_DECREF(pq->data);
      PyGILState_Release(gil);
      /* Free qstate */
      free(pq);
   }

   qstate->minfo[id] = NULL;
}

size_t pythonmod_get_mem(struct module_env* env, int id)
{
   struct pythonmod_env* pe = (struct pythonmod_env*)env->modinfo[id];
   verbose(VERB_ALGO, "pythonmod: get_mem, id: %d, pe:%lX", id,
   	(unsigned long int)pe);
   if(!pe)
      return 0;
   return sizeof(*pe);
}

/**
 * The module function block
 */
static struct module_func_block pythonmod_block = {
   "python",
   &pythonmod_init, &pythonmod_deinit, &pythonmod_operate, &pythonmod_inform_super,
   &pythonmod_clear, &pythonmod_get_mem
};

struct module_func_block* pythonmod_get_funcblock(void)
{
   return &pythonmod_block;
}
