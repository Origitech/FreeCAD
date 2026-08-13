#ifndef GUI_DOCKWINDOWMANAGERPY_H
#define GUI_DOCKWINDOWMANAGERPY_H

#include <Base/PyObjectBase.h>
#include <CXX/Extensions.hxx>
#include <FCGlobal.h>

namespace Gui
{

/**
 * Python front end of Gui::DockWindowManager, exposed as FreeCADGui.DockWindows.
 *
 * It lets Python code put a widget into the dock window registry so that the
 * regular workbench machinery (Workbench::setupDockWindows() and
 * DockWindowManager::setup()) can place, show, hide and persist it just like a
 * dock window created from C++. Without this a Python dock added straight to
 * the main window is invisible to setup(), saveState() and retranslate().
 *
 * Modelled on Gui::TaskView::ControlPy, which exposes Gui.Control the same way.
 */
class GuiExport DockWindowManagerPy: public Py::PythonExtension<DockWindowManagerPy>
{
public:
    static void init_type();  // announce properties and methods
    static DockWindowManagerPy* getInstance();

    DockWindowManagerPy();
    ~DockWindowManagerPy() override;

    Py::Object repr() override;

    Py::Object registerDockWindow(const Py::Tuple&);
    Py::Object adoptDockWindow(const Py::Tuple&);
    Py::Object unregisterDockWindow(const Py::Tuple&);
    Py::Object findRegisteredDockWindow(const Py::Tuple&);
    Py::Object getDockWindow(const Py::Tuple&);
    Py::Object getDockContainer(const Py::Tuple&);
    Py::Object getDockWindows(const Py::Tuple&);
    Py::Object activate(const Py::Tuple&);
    Py::Object saveState(const Py::Tuple&);
    Py::Object loadState(const Py::Tuple&);

private:
    static DockWindowManagerPy* instance;
};

}  // namespace Gui

#endif  // GUI_DOCKWINDOWMANAGERPY_H
