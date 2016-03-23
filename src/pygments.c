#include "Python.h"
#include "pygments.h"
#include <string.h>
#include <signal.h>

static PyObject *pyg;
static PyObject *pyg_formatters;
static PyObject *pyg_lexers;
static PyObject *formatter;

void pygments_init(void) {
	{
		struct sigaction sa;
		sigaction(SIGINT, NULL, &sa);
		Py_Initialize();
		sigaction(SIGINT, &sa, NULL);
	}
	pyg = PyImport_ImportModule("pygments");
	pyg_formatters = PyImport_ImportModule("pygments.formatters");
	pyg_lexers = PyImport_ImportModule("pygments.lexers");
	formatter = PyObject_CallMethod(pyg_formatters,
		"get_formatter_by_name", "s", "html");
}

char *pygmentize(const char *code, const char *language) {
	PyObject *lexer = PyObject_CallMethod(pyg_lexers, "get_lexer_by_name",
		"s", language);
	if(lexer == NULL) {
		PyErr_Clear();
		lexer = PyObject_CallMethod(pyg_lexers, "get_lexer_by_name",
			"s", "c");
		printf("bad language: %s\n", language);
	}
	PyObject *highlighted = PyObject_CallMethod(pyg, "highlight", "sOO", 
		code, lexer, formatter);
	Py_XDECREF(lexer);
	PyObject *utf8_encoded = PyObject_CallMethod(highlighted, "encode", "s", "utf-8");
	Py_XDECREF(highlighted);
	char *bytes = strdup(PyBytes_AsString(utf8_encoded));
	Py_XDECREF(utf8_encoded);
	return bytes;
}
