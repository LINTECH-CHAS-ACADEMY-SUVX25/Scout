# Runtime Improvements

Before any runtime improvements LVGL render =               400ms

Changes that affected runtime made between commits b63b097 to ac94e09

                                                            LVGL render time
1. Removed unused widgets in ui.c -                         400ms->370ms
2. Changed CPU frequency 240MHz (was 160MHz)                370ms->320ms
This and 3. also improved jpeg decode_task 75ms->40ms
3. Changed compiler optimization mode=PERF (was SIZE)       320ms->310ms
4. Changed LV_ATTRIBUTE_FAST_MEM_USE_IRAM=y                 310ms->210ms
5. Changed flash mode=QIO                                   210ms->200ms
6. Changed full_refresh=1 -> 0 from LVGL display driver     200ms->6-40ms 
Although lvgl render was reduced to 6-40ms we still need to wait for jpg decode and other tasks
before being able to render a new frame
6-40ms(lvgl)+~75ms(decode)+10ms(vtaskdelay)

lvgl render now only render changed areas instead of rendering the full screen every time 
ui_render_frame() is called which decreased render time but a huge amount.
Although now the lvgl render time is varied based on how many things are queued to be redrawn

7. Added partial LVGL draw buffer 1024x20 lines in SPIRAM (was full 1024x600 double framebuffer)
full_refresh=0 didn't work until I changed this

8. Pinned render_task and lvgl_tick to core 1, decode_task and tcp_server_task to core 0
This was largest improvement. This change moved lvgl render to one core and 
decode_task to another core which means they can work simultaneosly without waiting for 
each other.

9. Added direct framebuffer blit for camera frames bypassing LVGL canvas
bypassing LVGL canvas to draw the video_stream was necessary after change 8.
This made lvgl rendering and camera independant of eachother. Now we instantly display
the image whenever the jpg decode_task has finished.


10. Removed FPS counter overhead from TCP receive loop
Didn't notice an improvement after this, just thought it was unneccesary since lvgl 
provides an fps counter via sdkconfig

Before improvements a full rendering of the screen took around 500ms which meant we
had around 2fps

After improvements steady 33fps in lvgl
video stream estimated 30fps