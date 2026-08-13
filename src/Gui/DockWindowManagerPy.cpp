#include <string>

#include <QDockWidget>
#include <QWidget>

#include "DockWindowManagerPy.h"
#include "DockWindowManager.h"
#include "MainWindow.h"
#include "PythonWrapper.h"


using namespace Gui;

namespace
{

/// Unwrap a PySide widget, raising TypeError when it is anything else.
QWidget* toWidget(PyObject* pyObject, const char* argName)
{
    PythonWrapper wrap;
    wrap.loadWidgetsModule();

    QWidget* widget = qobject_cast<QWidget*>(wrap.toQObject(Py::Object(pyObject)));
    if (!widget) {
        throw Py::TypeError(std::string(argName) + " must be a QWidget");
    }
    return widget;
}

/// Wrap a widget for Python. Unlike PythonWrapper::fromQWidget this tolerates null.
Py::Object fromWidget(QWidget* widget, const char* className = nullptr)
{
    if (!widget) {
        return Py::None();
    }

    PythonWrapper wrap;
    wrap.loadWidgetsModule();
    return wrap.fromQWidget(widget, className);
}

}  // namespace

DockWindowManagerPy* DockWindowManagerPy::instance = nullptr;

DockWindowManagerPy* DockWindowManagerPy::getInstance()
{
    if (!instance) {
        instance = new DockWindowManagerPy();
    }
    return instance;
}

void DockWindowManagerPy::init_type()
{
    behaviors().name("DockWindows");
    behaviors().doc("Registry of the dock windows managed by FreeCAD");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();
    add_varargs_method(
        "registerDockWindow",
        &DockWindowManagerPy::registerDockWindow,
        "add a widget to the dock window registry\n"
        "registerDockWindow(name, widget) --> bool\n"
        "--\n"
        "The widget is the content of the dock window, not a QDockWidget: the\n"
        "container is created by FreeCAD when a workbench asks for the window.\n"
        "Give the widget the same objectName as the registration name, so that\n"
        "the saved main window layout finds it again on the next start.\n"
        "Returns False if the name is already taken. Ownership of the widget\n"
        "passes to FreeCAD."
    );
    add_varargs_method(
        "adoptDockWindow",
        &DockWindowManagerPy::adoptDockWindow,
        "take over a QDockWidget that was created and added elsewhere\n"
        "adoptDockWindow(name, dockWidget) --> bool\n"
        "--\n"
        "For docks that are a QDockWidget already instead of the content of\n"
        "one, which registerDockWindow cannot take. From then on the dock is\n"
        "managed like any other: workbenches can place it through their view\n"
        "list and its visibility is saved per workbench. It keeps its own\n"
        "object name, title and features.\n"
        "Returns False if it is already managed or holds no widget."
    );
    add_varargs_method(
        "unregisterDockWindow",
        &DockWindowManagerPy::unregisterDockWindow,
        "drop a widget from the dock window registry\n"
        "unregisterDockWindow(name)\n"
        "--\n"
        "Only removes the registry entry. A dock window already shown stays."
    );
    add_varargs_method(
        "findRegisteredDockWindow",
        &DockWindowManagerPy::findRegisteredDockWindow,
        "return a registered widget, whether or not it is shown\n"
        "findRegisteredDockWindow(name) --> QWidget or None"
    );
    add_varargs_method(
        "getDockWindow",
        &DockWindowManagerPy::getDockWindow,
        "return the content widget of a dock window that is currently shown\n"
        "getDockWindow(name) --> QWidget or None\n"
        "--\n"
        "Matches on the objectName of the dock window container."
    );
    add_varargs_method(
        "getDockContainer",
        &DockWindowManagerPy::getDockContainer,
        "return the QDockWidget holding the named dock window\n"
        "getDockContainer(name) --> QDockWidget or None"
    );
    add_varargs_method(
        "getDockWindows",
        &DockWindowManagerPy::getDockWindows,
        "return the content widgets of all dock windows that are shown\n"
        "getDockWindows() --> list of QWidget"
    );
    add_varargs_method(
        "activate",
        &DockWindowManagerPy::activate,
        "show and raise the dock window containing the given widget\n"
        "activate(widget)"
    );
    add_varargs_method(
        "saveState",
        &DockWindowManagerPy::saveState,
        "store the visibility of the dock windows of the active workbench\n"
        "saveState()"
    );
    add_varargs_method(
        "loadState",
        &DockWindowManagerPy::loadState,
        "restore the visibility of the dock windows of the active workbench\n"
        "loadState()"
    );
}

DockWindowManagerPy::DockWindowManagerPy() = default;

DockWindowManagerPy::~DockWindowManagerPy() = default;

Py::Object DockWindowManagerPy::repr()
{
    return Py::String("Dock window manager");
}

Py::Object DockWindowManagerPy::registerDockWindow(const Py::Tuple& args)
{
    char* name {};
    PyObject* pyWidget {};
    if (!PyArg_ParseTuple(args.ptr(), "sO", &name, &pyWidget)) {
        throw Py::Exception();
    }

    QWidget* widget = toWidget(pyWidget, "widget");
    bool added = DockWindowManager::instance()->registerDockWindow(name, widget);
    if (added) {
        // DockWindowManager::setup() reparents the widget into the QDockWidget it
        // creates for it. That happens on the C++ side where shiboken cannot see
        // it, so hand ownership over now: otherwise dropping the last Python
        // reference would destroy a widget the container still points at.
        PythonWrapper::setParent(pyWidget, getMainWindow());
    }

    return Py::Boolean(added);
}

Py::Object DockWindowManagerPy::adoptDockWindow(const Py::Tuple& args)
{
    char* name {};
    PyObject* pyDockWidget {};
    if (!PyArg_ParseTuple(args.ptr(), "sO", &name, &pyDockWidget)) {
        throw Py::Exception();
    }

    auto dw = qobject_cast<QDockWidget*>(toWidget(pyDockWidget, "dockWidget"));
    if (!dw) {
        throw Py::TypeError("dockWidget must be a QDockWidget");
    }

    // No ownership transfer here, unlike registerDockWindow: an adopted dock was created and
    // parented by its owner, so PySide already knows C++ holds it.
    return Py::Boolean(DockWindowManager::instance()->adoptDockWindow(name, dw));
}

Py::Object DockWindowManagerPy::unregisterDockWindow(const Py::Tuple& args)
{
    char* name {};
    if (!PyArg_ParseTuple(args.ptr(), "s", &name)) {
        throw Py::Exception();
    }

    // The widget itself is not returned: it stays owned by the main window.
    DockWindowManager::instance()->unregisterDockWindow(name);
    return Py::None();
}

Py::Object DockWindowManagerPy::findRegisteredDockWindow(const Py::Tuple& args)
{
    char* name {};
    if (!PyArg_ParseTuple(args.ptr(), "s", &name)) {
        throw Py::Exception();
    }

    return fromWidget(DockWindowManager::instance()->findRegisteredDockWindow(name));
}

Py::Object DockWindowManagerPy::getDockWindow(const Py::Tuple& args)
{
    char* name {};
    if (!PyArg_ParseTuple(args.ptr(), "s", &name)) {
        throw Py::Exception();
    }

    return fromWidget(DockWindowManager::instance()->getDockWindow(name));
}

Py::Object DockWindowManagerPy::getDockContainer(const Py::Tuple& args)
{
    char* name {};
    if (!PyArg_ParseTuple(args.ptr(), "s", &name)) {
        throw Py::Exception();
    }

    return fromWidget(DockWindowManager::instance()->getDockContainer(name), "QDockWidget");
}

Py::Object DockWindowManagerPy::getDockWindows(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), "")) {
        throw Py::Exception();
    }

    Py::List list;
    const QList<QWidget*> windows = DockWindowManager::instance()->getDockWindows();
    for (auto widget : windows) {
        list.append(fromWidget(widget));
    }

    return list;
}

Py::Object DockWindowManagerPy::activate(const Py::Tuple& args)
{
    PyObject* pyWidget {};
    if (!PyArg_ParseTuple(args.ptr(), "O", &pyWidget)) {
        throw Py::Exception();
    }

    DockWindowManager::instance()->activate(toWidget(pyWidget, "widget"));
    return Py::None();
}

Py::Object DockWindowManagerPy::saveState(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), "")) {
        throw Py::Exception();
    }

    DockWindowManager::instance()->saveState();
    return Py::None();
}

Py::Object DockWindowManagerPy::loadState(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), "")) {
        throw Py::Exception();
    }

    DockWindowManager::instance()->loadState();
    return Py::None();
}
