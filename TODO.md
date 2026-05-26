OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA OPTIMERA

Ta bort lvgl ksk? - lvgl är för bottar och npcs.


Mina tankar just nu 2026-26-05 //Alex

Ändra så att jpg decode inte är låsande
Uppdatera ALLTID skärmen med samma mellanrum för att inte "blockera" att kunna styra med joysticken medans man decodar

Skicka någon slags ping till Cam med samma mellanrum med data såsom körriktningskommando etc
om Cam inte fått ping på x antal ms så stängs alla motorer av

Sätta upp ett simplare sätt att kunna se hur alla tasks opererar, ksk en State machine worker lr liknande

wifi.c är för mig ett frågetecken. Jag misstänker att det är exempelkod för hur man startar ett "basic" wifi-AP
Vi kanske inte ens behöver starta ett wifi nätverk? räcker det med en tcp-server?
Orsaken varför jag bryr mig är för att tydligen tar wifi upp 300KB SRAM av 500KB tillgängligt vilket bara lämnar 100-200KB till jpg decode
Alltså kan vi inte ha så stor resolution ifall vi fortsätter att använda jpg decode i SRAM istället för PSRAM.

Sen kanske det finns bättre sätt än jag hittat att decoda via PSRAM? Men det verkar bottlenecka vid storlerkar större än 30KB
Matten är HOR_RES * VER_RES * 2 alltså för tillfället 240x176x2 = 84480 Byte 
Hur det funkar nu i screen/video.c tcp_server_task skriver jpg datan in i s_frame -> Jpeg decoder tar datan, omvandlar till rgb565
och skriver till -> s_canvas_buf som sedan displayas på nåt sätt

Både s_frame och s_canvas_buf ligger i SRAM. s_frame är storleken på paketet som skickas av CAM * 4. Så om paketen är i regel 2KB så bör den
sättas till 8KB för att vara säker. Sen s_canvas_buf är då 240x176x2 = 84480 Byte 85KB.

Min "vision" på programmets flowchart:

Uppdatera hela bildrutan med lvgl - dvs rita upp bakgrunden, joystick, telemetri, (alla lvgl moduler) 
samt uppdatera videostream ifall det finns en ny färdigdecodad frame.
skicka ping med styrdata från screen till cam
ta emot paket från Cam

Det kanske inte är det bästa sättet, vet inte hur man ska tänka om det går att göra vissa processer samtidigt.
Man kanske kan ta emot paket från cam medans man gör någonting annat, eller så kanske det är bäst att göra en sak i taget och låsa
andra grejer... 

