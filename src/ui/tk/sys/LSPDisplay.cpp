/*
 * LSPDisplay.cpp
 *
 *  Created on: 19 июн. 2017 г.
 *      Author: sadko
 */

#include <ui/ws/ws.h>
#include <ui/tk/tk.h>

#if defined(USE_X11_DISPLAY)
    #include <ui/ws/x11/ws.h>
#else
    #error "Unsupported platform"
#endif

namespace lsp
{
    namespace tk
    {
        LSPDisplay::LSPDisplay()
        {
            pDisplay        = NULL;
        }

        LSPDisplay::~LSPDisplay()
        {
            do_destroy();
        }
        
        void LSPDisplay::do_destroy()
        {
            // Auto-destruct widgets
            size_t n    = sWidgets.size();
            for (size_t i=0; i<n; ++i)
            {
                item_t *ptr     = sWidgets.at(i);
                if (ptr == NULL)
                    continue;

                ptr->id         = NULL;
                if (ptr->widget != NULL)
                {
                    ptr->widget->destroy();
                    delete ptr->widget;
                    ptr->widget = NULL;
                }
                ::free(ptr);
            }
            sWidgets.flush();

            // Execute slot
            sSlots.execute(LSPSLOT_DESTROY, NULL);
            sSlots.destroy();

            // Destroy display
            if (pDisplay != NULL)
            {
                pDisplay->destroy();
                delete pDisplay;
                pDisplay = NULL;
            }
        }

        status_t LSPDisplay::main_task_handler(ws::timestamp_t time, void *arg)
        {
            LSPDisplay *_this   = reinterpret_cast<LSPDisplay *>(arg);
            if (_this == NULL)
                return STATUS_BAD_ARGUMENTS;

            for (size_t i=0, n=_this->vGarbage.size(); i<n; ++i)
            {
                // Get widget
                LSPWidget *w = _this->vGarbage.at(i);
                if (w == NULL)
                    continue;

                // Widget is registered?
                for (size_t j=0, m=_this->sWidgets.size(); j<m; )
                {
                    item_t *item = _this->sWidgets.at(j);
                    if (item->widget == w)
                    {
                        // Free the binding
                        _this->sWidgets.remove(j, true);
                        item->id        = NULL;
                        item->widget    = NULL;
                        ::free(item);
                    }
                    else
                        ++j;
                }

                // Destroy widget
                w->destroy();
                delete w;
            }

            // Cleanup garbage
            _this->vGarbage.flush();

            return STATUS_OK;
        }

        status_t LSPDisplay::init(int argc, const char **argv)
        {
            // Create display dependent on the platform
            #ifdef USE_X11_DISPLAY
                pDisplay        = new x11::X11Display();
            #else
                #error "Unknown windowing system configuration"
            #endif /* USE_X11_DISPLAY */

            // Analyze display pointer
            if (pDisplay == NULL)
                return STATUS_NO_MEM;

            // Initialize display
            status_t result = pDisplay->init(argc, argv);
            if (result != STATUS_OK)
                return result;
            pDisplay->set_main_callback(main_task_handler, this);

            // Create slots
            LSPSlot *slot = sSlots.add(LSPSLOT_DESTROY);
            if (slot == NULL)
                return STATUS_NO_MEM;
            slot = sSlots.add(LSPSLOT_RESIZE);
            if (slot == NULL)
                return STATUS_NO_MEM;

            // Initialize theme
            sTheme.init(this);

            return STATUS_OK;
        }

        void LSPDisplay::destroy()
        {
            do_destroy();
        }

        status_t LSPDisplay::main()
        {
            if (pDisplay == NULL)
                return STATUS_BAD_STATE;
            return pDisplay->main();
        }

        status_t LSPDisplay::main_iteration()
        {
            if (pDisplay == NULL)
                return STATUS_BAD_STATE;
            return pDisplay->main_iteration();
        }

        void LSPDisplay::quit_main()
        {
            if (pDisplay != NULL)
                pDisplay->quit_main();
        }

        ISurface *LSPDisplay::create_surface(size_t width, size_t height)
        {
            return (pDisplay != NULL) ? pDisplay->createSurface(width, height) : NULL;
        }

        void LSPDisplay::sync()
        {
            if (pDisplay != NULL)
                pDisplay->sync();
        }

        status_t LSPDisplay::add(LSPWidget *widget, const char *id)
        {
            LSPWidget **w = add(id);
            if (w == NULL)
                return STATUS_NO_MEM;

            *w = widget;
            return STATUS_OK;
        }

        LSPWidget **LSPDisplay::add(const char *id)
        {
            // Prevent from duplicates
            if (id != NULL)
            {
                // Check that widget does not exist
                LSPWidget *widget = get(id);
                if (widget != NULL)
                    return NULL;
            }

            // Allocate memory
            size_t slen     = (id != NULL) ? (::strlen(id) + 1) * sizeof(char) : 0;
            size_t to_alloc = ALIGN_SIZE(sizeof(item_t) + slen, DEFAULT_ALIGN);

            // Append widget
            item_t *w   = reinterpret_cast<item_t *>(::malloc(to_alloc));
            if (w == NULL)
                return NULL;
            else if (!sWidgets.add(w))
            {
                ::free(w);
                return NULL;
            }

            // Initialize widget
            w->widget       = NULL;
            w->id           = NULL;
            if (id != NULL)
            {
                w->id           = reinterpret_cast<char *>(&w[1]);
                ::memcpy(w->id, id, slen);
            }

            return &w->widget;
        }

        LSPWidget *LSPDisplay::get(const char *id)
        {
            if (id == NULL)
                return NULL;

            size_t n    = sWidgets.size();
            for (size_t i=0; i<n; ++i)
            {
                item_t *ptr     = sWidgets.at(i);
                if (ptr->id == NULL)
                    continue;
                if (!strcmp(ptr->id, id))
                    return ptr->widget;
            }

            return NULL;
        }

        LSPWidget *LSPDisplay::remove(const char *id)
        {
            if (id == NULL)
                return NULL;

            size_t n    = sWidgets.size();
            for (size_t i=0; i<n; ++i)
            {
                item_t *ptr     = sWidgets.at(i);
                if (ptr->id == NULL)
                    continue;
                if (!strcmp(ptr->id, id))
                {
                    LSPWidget *result = ptr->widget;
                    sWidgets.remove(i);
                    return result;
                }
            }

            return NULL;
        }

        bool LSPDisplay::remove(LSPWidget *widget)
        {
            size_t n    = sWidgets.size();
            for (size_t i=0; i<n; ++i)
            {
                item_t *ptr     = sWidgets.at(i);
                if (ptr->widget == widget)
                {
                    sWidgets.remove(i);
                    return true;
                }
            }

            return false;
        }

        bool LSPDisplay::exists(LSPWidget *widget)
        {
            size_t n    = sWidgets.size();
            for (size_t i=0; i<n; ++i)
            {
                item_t *ptr     = sWidgets.at(i);
                if (ptr->widget == widget)
                    return true;
            }

            return false;
        }

        status_t LSPDisplay::get_clipboard(size_t id, IDataSink *sink)
        {
            return pDisplay->getClipboard(id, sink);
        }

        status_t LSPDisplay::set_clipboard(size_t id, IDataSource *src)
        {
            return pDisplay->setClipboard(id, src);
        }

        status_t LSPDisplay::reject_drag()
        {
            return pDisplay->rejectDrag();
        }

        status_t LSPDisplay::accept_drag(IDataSink *sink, drag_t action, bool internal, const realize_t *r)
        {
            return pDisplay->acceptDrag(sink, action, internal, r);
        }

        const char * const *LSPDisplay::get_drag_mime_types()
        {
            return pDisplay->getDragContentTypes();
        }

        status_t LSPDisplay::queue_destroy(LSPWidget *widget)
        {
            return vGarbage.add(widget) ? STATUS_OK : STATUS_NO_MEM;
        }
    }

} /* namespace lsp */
