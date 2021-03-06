
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2010-2013 Francois Beaune, Jupiter Jazz Limited
// Copyright (c) 2014-2017 Francois Beaune, The appleseedhq Organization
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#ifndef APPLESEED_STUDIO_MAINWINDOW_RENDERING_RENDERINGMANAGER_H
#define APPLESEED_STUDIO_MAINWINDOW_RENDERING_RENDERINGMANAGER_H

// appleseed.studio headers.
#include "mainwindow/rendering/frozendisplayrenderer.h"
#include "mainwindow/rendering/qtrenderercontroller.h"
#include "mainwindow/rendering/qttilecallback.h"
#include "mainwindow/rendering/renderingtimer.h"

// appleseed.renderer headers.
#include "renderer/api/rendering.h"
#include "renderer/api/types.h"
#include "renderer/api/utility.h"

// appleseed.foundation headers.
#include "foundation/math/transform.h"
#include "foundation/platform/thread.h"
#include "foundation/utility/autoreleaseptr.h"
#include "foundation/utility/job/abortswitch.h"

// Qt headers.
#include <QObject>
#include <QThread>

// Standard headers.
#include <map>
#include <memory>
#include <string>
#include <vector>

// Forward declarations.
namespace appleseed     { namespace studio { class RenderTab; } }
namespace appleseed     { namespace studio { class StatusBar; } }
namespace foundation    { class IAbortSwitch; }
namespace renderer      { class Frame; }
namespace renderer      { class Project; }

namespace appleseed {
namespace studio {

class RenderingManager
  : public QObject
{
    Q_OBJECT

  public:
    // Constructor.
    explicit RenderingManager(StatusBar& status_bar);

    // Destructor.
    ~RenderingManager();

    // Start rendering.
    void start_rendering(
        renderer::Project*              project,
        const renderer::ParamArray&     params,
        RenderTab*                      render_tab);

    // Return true if currently rendering, false otherwise.
    bool is_rendering() const;

    // Wait until rendering has ended.
    void wait_until_rendering_end();

    // Send orders to the renderer via the renderer controller.
    void abort_rendering();
    void restart_rendering();
    void reinitialize_rendering();
    void pause_rendering();
    void resume_rendering();

    // Interface for scheduled actions.
    class IScheduledAction
    {
      public:
        virtual ~IScheduledAction() {}

        virtual void operator()(
            renderer::Project&          project) = 0;
    };

    // Schedule an action for execution.
    // Actions are executed once, right before rendering begins, in the order in which
    // they were scheduled. They are then deleted.
    void schedule(std::auto_ptr<IScheduledAction> action);

    // Schedule an action for execution if currently rendering, or execute the action
    // right away if not.
    void schedule_or_execute(std::auto_ptr<IScheduledAction> action);

    // Remove all actions scheduled since rendering has begun.
    void clear_scheduled_actions();

    // Interface for sticky actions.
    class IStickyAction
    {
      public:
        virtual ~IStickyAction() {}

        virtual void operator()(
            renderer::MasterRenderer&   master_renderer,
            renderer::Project&          project) = 0;
    };

    // Add or replace a sticky action associated with a given (arbitrary) key.
    // A sticky action is one that is executed every time rendering starts.
    // Sticky actions remain active until explicitly replaced or deleted.
    // There are no guarantees regarding the order of execution of sticky actions.
    void set_sticky_action(
        const std::string&              key,
        std::auto_ptr<IStickyAction>    action);

    // Remove all sticky actions.
    void clear_sticky_actions();

  signals:
    void signal_camera_changed();
    void signal_rendering_end();

  public slots:
    void slot_abort_rendering();
    void slot_restart_rendering();
    void slot_reinitialize_rendering();

  private:
    StatusBar&                                  m_status_bar;
    QtRendererController                        m_renderer_controller;

    renderer::Project*                          m_project;
    renderer::ParamArray                        m_params;
    RenderTab*                                  m_render_tab;

    std::auto_ptr<QtTileCallbackFactory>        m_tile_callback_factory;
    std::auto_ptr<renderer::MasterRenderer>     m_master_renderer;
    std::auto_ptr<QThread>                      m_master_renderer_thread;

    RenderingTimer                              m_rendering_timer;

    typedef std::vector<IScheduledAction*> ScheduledActionCollection;
    typedef std::map<std::string, IStickyAction*> StickyActionCollection;

    ScheduledActionCollection                   m_scheduled_actions;
    StickyActionCollection                      m_sticky_actions;

    bool                                        m_has_camera_changed;

    class FrozenDisplayFunc
    {
      public:
        FrozenDisplayFunc(
            const renderer::SamplingContext::Mode   sampling_mode,
            const renderer::Camera&                 camera,
            const renderer::Frame&                  frame,
            renderer::ITileCallbackFactory&         tile_callback_factory,
            foundation::IAbortSwitch&               abort_switch);

        void set_camera_transform(
            const foundation::Transformd&           transform);

        void operator()();

      private:
        FrozenDisplayRenderer                                   m_renderer;
        const renderer::Frame&                                  m_frame;
        foundation::auto_release_ptr<renderer::ITileCallback>   m_tile_callback;
        foundation::IAbortSwitch&                               m_abort_switch;
    };

    std::auto_ptr<FrozenDisplayFunc>            m_frozen_display_func;
    std::auto_ptr<boost::thread>                m_frozen_display_thread;
    foundation::AbortSwitch                     m_frozen_display_abort_switch;

    void print_final_rendering_time();
    void print_average_luminance();
    void archive_frame_to_disk();

    void run_scheduled_actions();
    void run_sticky_actions();

  private slots:
    void slot_rendering_begin();
    void slot_rendering_end();
    void slot_frame_begin();
    void slot_frame_end();
    void slot_camera_change_begin();
    void slot_camera_changed();
    void slot_camera_change_end();
    void slot_master_renderer_thread_finished();
};

}       // namespace studio
}       // namespace appleseed

#endif  // !APPLESEED_STUDIO_MAINWINDOW_RENDERING_RENDERINGMANAGER_H
