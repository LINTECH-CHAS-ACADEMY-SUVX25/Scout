OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA 

# Hög prio:

### Kolla vad som saknas enligt kurskraven
UART I2C med mera

### Simplifiera flödet - rensa ur svårlästa hårdkodade funktioner ur programmets "flowchart"
    scout_screen:main->wifi->display->ui->stream->render
    scout_cam:main->motor->wifi->camera->?telemetry?->motor_task->stream
    ^ kanske ändra så den följer samma ordning som screen
Alla dessa filer bör vara enkla att läsa, inte innehålla helper eller 
hårdkodade funktioner t.ex. socket bindningar via tcp/udp

### scout_cam måste skrivas om så det följer ett liknande flöde som screen eller tvärtom
i screen så anropas task_start i inits medans i cam så anropas t.ex. motor_task_start() i main

### Skicka logging från cam->screen via UDP
vi har en massa esp_log funktioner i cam som aldrig ens kan läsas eftersom vi inte har UART

### Bestäm frame size
just nu 2026-06-05 så kör vi på 640x480 och den känns ganska stor. 
tyvärr är nästa mindre "officiela" storleken CIF=352x288 ganska mycket mindre.
jag gillar höjden 480px men bredden känns lite för lång. Vi borde kunna "skära av" 80px
höger och vänster om bredden 640px så skickar vi en custom res på 480x480 istället

### Filstruktur
Det är rörigt med alla filer som ligger direkt i main mappen
Såhär vill jag ha det:
Scout/
    README.md
    LICENSE
    CHANGELOG.md
    shared_components/ <- delade komponenter
        udp/
        rc_protocol/
    scout_cam/
        CMakeLists.txt
        sdkconfig.defaults
        main/
        components/ <- cam komponenter
            camera/
            motor_driver/
            etc...
    scout_screen/
        CMakeLists.txt
        sdkconfig.defaults
        main/
        components/ <- screen komponenter
            jpeg_decode/
            lvgl_port/
            etc...
    test/
    docs/
        architecture.md
        protocols/
        diagrams/
        hardware/

# Mid prio:
### Protokoll för logging 
projektet har slumpmässiga esp_log funktioner här och där

### Lägg till en buffer till jpeg_decode? 
Kanske kan öka fps här men det kanske ökar "delay"

### cam->telemetry.c är placeholder
Ska skicka sensorvärden
vi kan också skicka RSSI/signalstyrka "esp_wifi_sta_get_ap_info()" finns genom 
cam->wifi->esp_wifi

### Fixa lvgl ui
video_streamen fyller inte rutan lvgl ritar upp just nu

### Samla ihop defines på ett enkelt sätt
rc_protokoll finns ju redan men snackar om t.ex. "#define CAM_X ((SCREEN_W - CAM_W) / 2)" som ligger i render.c

# Låg prio:
Protokoll för kommentarer 
många funktioner har inga kommentarer där dom borde ha
en del kommentarer är på engelska, en del svenska

### Ändra config utan flash
Undersök om man kan ändra vissa inställningar typ camera_config_t->.frame_size .jpeg_quality utan att behöva flasha igen
