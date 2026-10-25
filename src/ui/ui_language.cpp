#include "ui/ui_language.h"
#include "config.h"
#include <Preferences.h>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <algorithm>

namespace {
UILanguage::Language language = UILanguage::Language::English;
struct Entry { const char *en; const char *tr; };
const Entry catalog[] = {
    {"\nControl", "\nKontrol"},
    {"\nFiles", "\nDosyalar"},
    {"\nMacros", "\nMakrolar"},
    {"\nSettings", "\nAyarlar"},
    {"\nStatus", "\nDurum"},
    {"\nTerminal", "\nTerminal"},
    {"  Excellent", "  Çok iyi"},
    {"  Fair", "  Orta"},
    {"  Good", "  İyi"},
    {"  Weak", "  Zayıf"},
    {" %s (%s) is Locked", " %s (%s) kilitli"},
    {" %s is Locked", " %s kilitli"},
    {" Add", " Ekle"},
    {" All", " Tümü"},
    {" Cancel Jog", " Hareketi İptal Et"},
    {" Check value", " Değeri kontrol edin"},
    {" Clear & Restart", " Sil ve Yeniden Başlat"},
    {" Clear Alarm", " Alarmı Temizle"},
    {" Clear All", " Tümünü Sil"},
    {" Clear All Settings?", " Tüm Ayarlar Silinsin mi?"},
    {" Close", " Kapat"},
    {" Continue", " Devam"},
    {" Delete", " Sil"},
    {" Done", " Tamam"},
    {" Edit", " Düzenle"},
    {" Edit to add macros.", " Makro eklemek için düzenleyin."},
    {" Edit to add macros.\n\nMacro files must be on machine SD card in /fluidtouch/macros directory.", " Makro eklemek için düzenleyin.\n\nMakro dosyaları makinenin SD kartında /fluidtouch/macros klasöründe olmalıdır."},
    {" Export", " Dışa Aktar"},
    {" Export Successful", " Dışa Aktarıldı"},
    {" Machine alarm", " Makine alarmı"},
    {" Machine on hold", " Makine beklemede"},
    {" Pause", " Duraklat"},
    {" Power", " Güç"},
    {" Prev", " Önceki"},
    {" Refresh", " Yenile"},
    {" Restart", " Yeniden Başlat"},
    {" Restart Required", " Yeniden Başlatılmalı"},
    {" Resume", " Sürdür"},
    {" Retry", " Tekrar Dene"},
    {" STOP", " DUR"},
    {" Scan", " Tara"},
    {" Sleep", " Uyut"},
    {" Soft Reset", " Yazılımı Sıfırla"},
    {" System Options", " Sistem Seçenekleri"},
    {" UPLOAD COMPLETE", " YÜKLEME TAMAMLANDI"},
    {" Unlock", " Kilidi Ağ"},
    {" Upload", " Yükle"},
    {" Upload cancelled", " Yükleme iptal edildi"},
    {" Upload failed", " Yükleme başarısız"},
    {" Upload to FluidNC", " FluidNC'ye Yükle"},
    {" Uploading...", " Yükleniyor..."},
    {" WiFi Password Required", " WiFi Parolası Gerekli"},
    {" WiFi passwords are NOT included", " Güvenlik nedeniyle WiFi parolaları"},
    {" WiFi passwords are NOT included\nin the backup file for security reasons.", " Güvenlik nedeniyle WiFi parolaları\nyedek dosyasına DAHİL EDİLMEZ."},
    {"%d bytes / %d bytes (%d%%)", "%d bayt / %d bayt (%d%%)"},
    {"%d file(s)", "%d dosya"},
    {"%d folder(s)", "%d klasör"},
    {"%d folder(s), %d file(s)", "%d klasör, %d dosya"},
    {"%d saved machines", "%d kayıtlı makine"},
    {"%s", "%s"},
    {"%s (%s) is Locked", "%s (%s) kilitli"},
    {"%s Delete File?", "%s Dosya Silinsin mi?"},
    {"%s Delete Macro?", "%s Makro Silinsin mi?"},
    {"%s is Locked", "%s kilitli"},
    {"%s: %.3f mm", "%s: %.3f mm"},
    {"(comma separated, max 5)", "(virgülle ayırın, en fazla 5)"},
    {"-- Select a file --", "-- Dosya seçin --"},
    {"/fluidtouch_settings.json", "/fluidtouch_settings.json"},
    {"1 min", "1 dk"},
    {"10 min", "10 dk"},
    {"15 min", "15 dk"},
    {"15 sec", "15 sn"},
    {"2  Machine", "2  Makine"},
    {"2 min", "2 dk"},
    {"30 min", "30 dk"},
    {"30 sec", "30 sn"},
    {"45 sec", "45 sn"},
    {"5 min", "5 dk"},
    {"60 min", "60 dk"},
    {"60 sec", "60 sn"},
    {"90 min", "90 dk"},
    {"90 sec", "90 sn"},
    {"A Feed:", "A Hız:"},
    {"A JOG", "A HAREKET"},
    {"A Step", "A Adım"},
    {"A Steps: %s", "A Adımlar: %s"},
    {"A-axis setting requires a restart", "A ekseni ayarını uygulamak için"},
    {"A-axis setting requires a restart\nto take effect.\n\nRestart now?", "A ekseni ayarını uygulamak için\nyeniden başlatma gerekiyor.\n\nŞimdi yeniden başlatılsın mı?"},
    {"ALARM", "ALARM"},
    {"AXES", "EKSENLER"},
    {"About", "Hakkında"},
    {"Actions", "İşlemler"},
    {"Add", "Ekle"},
    {"Add Machine", "Makine Ekle"},
    {"Add Macro", "Makro Ekle"},
    {"Adds A-axis controls and display.", "A ekseni kontrol ve göstergelerini ekler."},
    {"Alarm condition detected", "Alarm durumu algılandı"},
    {"All", "Tümü"},
    {"Applies after Save Settings.", "Ayarları Kaydet ile uygulanır."},
    {"Are you sure you want to continue?", "Devam etmek istediğinizden emin misiniz?"},
    {"At root", "Ana dizindesiniz"},
    {"Auto Scroll", "Otomatik Kaydır"},
    {"Available while the machine is idle.", "Makine boştayken kullanılabilir."},
    {"Axis", "Eksen"},
    {"BACKUP & RESTORE", "YEDEKLEME VE GERİ YÜKLEME"},
    {"Backup", "Yedekleme"},
    {"Brightness:", "Parlaklık:"},
    {"CHECK", "KONTROL"},
    {"CONNECTION", "BAĞLANTI"},
    {"CONTROL", "KONTROL"},
    {"COOLANT", "SOĞUTMA"},
    {"Cancel", "İptal"},
    {"Cancel Jog", "Hareketi İptal Et"},
    {"Check WiFi settings and try again.", "WiFi ayarlarını kontrol edip tekrar deneyin."},
    {"Check network settings and try again.", "Ağ ayarlarını kontrol edip tekrar deneyin."},
    {"Check that the machine is powered on", "Makinenin açık olduğundan ve"},
    {"Check the connection, then try again.", "Bağlantıyı kontrol edip tekrar deneyin."},
    {"Check value", "Değeri kontrol edin"},
    {"Clear", "Temizle"},
    {"Clear & Restart", "Sil ve Yeniden Başlat"},
    {"Clear Alarm", "Alarmı Temizle"},
    {"Clear All", "Tümünü Sil"},
    {"Clear All Settings", "Tüm Ayarları Sil"},
    {"Clear All Settings?", "Tüm Ayarlar Silinsin mi?"},
    {"Clear All erases all settings and restarts the device.", "Tümünü Sil, tüm ayarları silip cihazı yeniden başlatır."},
    {"Clearing settings...", "Ayarlar siliniyor..."},
    {"Click", "Dokunun:"},
    {"Click ", "Dokunun: "},
    {"Click Connect to reconnect.", "Yeniden bağlanmak için Bağlan'a dokunun."},
    {"Click Refresh", "Yenile'ye dokunun"},
    {"Close", "Kapat"},
    {"Color:", "Renk:"},
    {"Complete", "Tamamlandı"},
    {"Connect", "Bağlan"},
    {"Connected", "Bağlandı"},
    {"Connecting to WiFi...", "WiFi'ye bağlanılıyor..."},
    {"Connecting to machine...", "Makineye bağlanılıyor..."},
    {"Connection", "Bağlantı"},
    {"Connection Failed", "Bağlantı Başarısız"},
    {"Connection Lost", "Bağlantı Kesildi"},
    {"Connection error", "Bağlantı hatası"},
    {"Connection failed", "Bağlantı kurulamadı"},
    {"Continue", "Devam"},
    {"Control", "Kontrol"},
    {"Could not connect to machine:", "Makineye bağlanılamadı:"},
    {"Could not connect to machine:\n%s\n\nURL: %s:%d\n\nCheck that the machine is powered on\nand network connection is available.", "Makineye bağlanılamadı:\n%s\n\nAdres: %s:%d\n\nMakinenin açık olduğundan ve\nağ bağlantısından emin olun."},
    {"Could not connect to network:", "Ağa bağlanılamadı:"},
    {"Could not connect to network:\n%s\n\nCheck WiFi settings and try again.", "Ağa bağlanılamadı:\n%s\n\nWiFi ayarlarını kontrol edip tekrar deneyin."},
    {"Could not reconnect to WiFi.", "WiFi'ye yeniden bağlanılamadı."},
    {"Could not reconnect to WiFi.\n\nCheck network settings and try again.", "WiFi'ye yeniden bağlanılamadı.\n\nAğ ayarlarını kontrol edip tekrar deneyin."},
    {"Custom name", "Özel ad"},
    {"DISCONNECTED", "BAĞLI DEĞİL"},
    {"DISPLAY", "EKRAN"},
    {"DIST", "MESAFE"},
    {"DOOR", "KAPI"},
    {"Delete", "Sil"},
    {"Delete Machine?", "Makine Silinsin mi?"},
    {"Destination: /sd%s/%s", "Hedef: /sd%s/%s"},
    {"Dim After:", "Kararma:"},
    {"Dim Level:", "Kısık Işık:"},
    {"Disabled", "Kapalı"},
    {"Disabled\n1 min\n2 min\n5 min\n10 min\n15 min\n30 min\n60 min", "Kapalı\n1 dk\n2 dk\n5 dk\n10 dk\n15 dk\n30 dk\n60 dk"},
    {"Disabled\n15 sec\n30 sec\n45 sec\n60 sec\n90 sec\n2 min\n5 min", "Kapalı\n15 sn\n30 sn\n45 sn\n60 sn\n90 sn\n2 dk\n5 dk"},
    {"Disabled\n5 min\n10 min\n15 min\n30 min\n60 min\n90 min", "Kapalı\n5 dk\n10 dk\n15 dk\n30 dk\n60 dk\n90 dk"},
    {"Disconnect", "Bağlantıyı Kes"},
    {"Disconnected", "Bağlı değil"},
    {"Display SD", "Ekran SD"},
    {"Display rotation requires a restart", "Ekran yönünü uygulamak için"},
    {"Display rotation requires a restart\nto take effect.\n\nRestart now?", "Ekran yönünü uygulamak için\nyeniden başlatma gerekiyor.\n\nŞimdi yeniden başlatılsın mı?"},
    {"Documentation & Source Code", "Belgeler ve Kaynak Kod"},
    {"Done", "Tamam"},
    {"ERROR", "HATA"},
    {"EXPORTED SETTINGS", "DIŞA AKTARILAN AYARLAR"},
    {"Edit", "Düzenle"},
    {"Edit Machine", "Makineyi Düzenle"},
    {"Edit Macro", "Makroyu Düzenle"},
    {"Edit to add macros.", "Makro eklemek için düzenleyin."},
    {"Edit to add macros.\n\nMacro files must be on machine SD card in /fluidtouch/macros directory.", "Makro eklemek için düzenleyin.\n\nMakro dosyaları makinenin SD kartında /fluidtouch/macros klasöründe olmalıdır."},
    {"Elapsed", "Geçen"},
    {"Empty value in list", "Listede boş değer var"},
    {"Enable A-Axis:", "A Ekseni:"},
    {"Enabled:", "Etkin:"},
    {"Enables 4th axis (rotary) support.", "4. eksen (döner) desteğini açar."},
    {"Enables 4th axis (rotary) support.\nAdds A-axis controls and display.", "4. eksen (döner) desteğini açar.\nA ekseni kontrol ve göstergelerini ekler."},
    {"Enter", "Giriş"},
    {"Enter a machine name.", "Bir makine adı girin."},
    {"Enter command...", "Komut girin..."},
    {"Enter the FluidNC host name or IP address.", "FluidNC ana bilgisayar adını veya IP adresini girin."},
    {"Error parsing file list", "Dosya listesi okunamadı"},
    {"Error: %s", "Hata: %s"},
    {"Error: Failed to save!", "Hata: Kaydedilemedi!"},
    {"Error: Invalid feed rate or distance", "Hata: Geçersiz hız veya mesafe"},
    {"Error: Not connected to FluidNC", "Hata: FluidNC bağlantısı yok"},
    {"Estimated", "Tahmini"},
    {"Excellent", "Çok iyi"},
    {"Export", "Dışa Aktar"},
    {"Export Successful", "Dışa Aktarıldı"},
    {"Export failed! Check SD card.", "Dışa aktarılamadı! SD kartı kontrol edin."},
    {"Export saves a backup file to the Display SD card.", "Dışa Aktar, ekranın SD kartına yedek kaydeder."},
    {"Export saves a backup file to the Display SD card.\nClear All erases all settings and restarts the device.", "Dışa Aktar, ekranın SD kartına yedek kaydeder.\nTümünü Sil, tüm ayarları silip cihazı yeniden başlatır."},
    {"Exporting...", "Dışa aktarılıyor..."},
    {"FAILED", "BAŞARISIZ"},
    {"FAILED\nNo contact detected", "BAŞARISIZ\nTemas algılanmadı"},
    {"FEED", "İLERLEME"},
    {"FEED RATE", "İLERLEME HIZI"},
    {"FILES", "DOSYALAR"},
    {"Failed to open local file", "Yerel dosya açılamadı"},
    {"Failed to resolve hostname", "Ana bilgisayar adı çözümlenemedi"},
    {"Fair", "Orta"},
    {"Feed", "Hız"},
    {"Feed Rate (mm/min):", "İlerleme Hızı (mm/min):"},
    {"Feed Rate:", "İlerleme Hızı:"},
    {"File list timed out. Refresh to retry.", "Dosya listesi zaman aşımına uğradı. Yenileyin."},
    {"File list too large. Use smaller folders.", "Dosya listesi Çok büyük. Daha küçük klasörler kullanın."},
    {"File location: Display SD card root", "Konum: Ekranın SD kartı ana dizini"},
    {"File too large (max 10MB)", "Dosya Çok büyük (en fazla 10 MB)"},
    {"File: %s", "Dosya: %s"},
    {"File: /sd/fluidtouch/macros/", "Dosya: /sd/fluidtouch/macros/"},
    {"Files", "Dosyalar"},
    {"Files: /sd/fluidtouch/macros/", "Dosyalar: /sd/fluidtouch/macros/"},
    {"FluidNC Flash", "FluidNC Flash"},
    {"FluidNC SD", "FluidNC SD"},
    {"FluidNC SD\nFluidNC Flash\nDisplay SD", "FluidNC SD\nFluidNC Flash\nEkran SD"},
    {"FluidNC URL", "FluidNC Adresi"},
    {"FluidNC Version: %s", "FluidNC Sürümü: %s"},
    {"FluidNC not connected", "FluidNC bağlı değil"},
    {"Folders on Top:", "Klasörler Üstte:"},
    {"General", "Genel"},
    {"Good", "İyi"},
    {"HOLD", "BEKLEME"},
    {"HOME", "REFERANS"},
    {"HOME AXIS", "REFERANSA GİT"},
    {"Hold the screen for 3 seconds to wake.", "Uyandırmak için ekrana 3 saniye basılı tutun."},
    {"Hold the screen for 3 seconds to wake.\nAvailable while the machine is idle.", "Uyandırmak için ekrana 3 saniye basılı tutun.\nMakine boştayken kullanılabilir."},
    {"IDLE", "BOŞTA"},
    {"IDLE / OFFLINE only. Hold the screen for 3 seconds to wake from standby.", "Yalnızca BOŞTA / ÇEVRİMDIŞI. Uyandırmak için 3 saniye basılı tutun."},
    {"Invalid character in number", "Sayıda geçersiz karakter var"},
    {"Invalid number entered", "Geçersiz sayı girildi"},
    {"Invalid number format", "Geçersiz sayı biçimi"},
    {"Invalid upload path or disconnected machine", "Geçersiz yükleme yolu veya makine bağlı değil"},
    {"JOG", "HAREKET"},
    {"JOG & JOYSTICK CONTROL DEFAULTS", "HAREKET VE JOYSTICK VARSAYILANLARI"},
    {"Jog", "Hareket"},
    {"Joystick", "Joystick"},
    {"Language", "Dil"},
    {"Language saved. Restarting...", "Dil kaydedildi. Yeniden başlatılıyor..."},
    {"Later", "Sonra"},
    {"Loading files...", "Dosyalar yükleniyor..."},
    {"Loading...", "Yükleniyor..."},
    {"MACHINE POSITION", "MAKİNE KONUMU"},
    {"MACHINE SELECTION", "MAKİNE SEÇİMİ"},
    {"MACHINE SETUP", "MAKİNE AYARLARI"},
    {"MESSAGE", "MESAJ"},
    {"MODAL", "KİPLER"},
    {"MOTION", "HAREKET"},
    {"Machine", "Makine"},
    {"Machine Connection Failed", "Makine Bağlantısı Başarısız"},
    {"Machine alarm", "Makine alarmı"},
    {"Machine is disconnected.", "Makine bağlı değil."},
    {"Machine is disconnected.\n\n%s\nURL: %s:%d\n\nClick Connect to reconnect.", "Makine bağlı değil.\n\n%s\nAdres: %s:%d\n\nYeniden bağlanmak için Bağlan'a dokunun."},
    {"Machine must be IDLE to list files", "Dosya listelemek için makine BOŞTA olmalı"},
    {"Machine on hold", "Makine beklemede"},
    {"Machine paused", "Makine duraklatıldı"},
    {"Machine: %s", "Makine: %s"},
    {"Machine: %s\n\n", "Makine: %s\n\n"},
    {"Macro files must be on machine SD card in /fluidtouch/macros directory.", "Makro dosyaları makinenin SD kartında /fluidtouch/macros klasöründe olmalıdır."},
    {"Macros", "Makrolar"},
    {"Manage Machines", "Makineleri Yönet"},
    {"Max", "Azami"},
    {"Max Distance (mm):", "Azami Mesafe (mm):"},
    {"Max Distance:", "Azami Mesafe:"},
    {"Max: %d mm/min", "Azami: %d mm/min"},
    {"Maximum 5 values allowed", "En fazla 5 değer girilebilir"},
    {"Modifying this position may affect fixtures or setups.", "Bu konumu değiştirmek bağlama düzenini etkileyebilir."},
    {"Modifying this position may affect fixtures or setups.\n\nAre you sure you want to continue?", "Bu konumu değiştirmek bağlama düzenini etkileyebilir.\n\nDevam etmek istediğinizden emin misiniz?"},
    {"Name", "Ad"},
    {"Name:", "Ad:"},
    {"Next", "Sonraki"},
    {"Next ", "Sonraki "},
    {"No Machine", "Makine Yok"},
    {"No contact detected", "Temas algılanmadı"},
    {"No files found", "Dosya bulunamadı"},
    {"No files on SD card", "SD kartta dosya yok"},
    {"No macros configured.", "Makro tanımlanmamış."},
    {"No macros configured.\n\nClick", "Makro tanımlanmamış.\n\nDokunun:"},
    {"No macros configured.\n\nClick ", "Makro tanımlanmamış.\n\nDokunun: "},
    {"No macros configured.\n\nUse Edit to add macros.\nFiles: /sd/fluidtouch/macros/", "Makro tanımlanmamış.\n\nMakro eklemek için Düzenle'ye dokunun.\nDosyalar: /sd/fluidtouch/macros/"},
    {"No messages.", "Mesaj yok."},
    {"No networks found", "Ağ bulunamadı"},
    {"No probe data", "Prob verisi yok"},
    {"No saved machines. Tap Edit, then Add.", "Kayıtlı makine yok. Düzenle, ardından Ekle'ye dokunun."},
    {"No valid values found", "Geçerli değer bulunamadı"},
    {"Not Connected", "Bağlı Değil"},
    {"Not connected", "Bağlı değil"},
    {"Not enough memory for file list.", "Dosya listesi için yeterli bellek yok."},
    {"Not enough memory to start upload", "Yükleme başlatmak için yeterli bellek yok"},
    {"OFFLINE", "ÇEVRİMDIŞI"},
    {"OK", "Tamam"},
    {"Open Upload Folder", "Yükleme Klasörünü Ağ"},
    {"Out of memory", "Bellek yetersiz"},
    {"Overrides", "Hız Oranları"},
    {"PARAMETERS", "PARAMETRELER"},
    {"PLANE", "DÜZLEM"},
    {"POWER", "GÜÇ"},
    {"POWER MANAGEMENT", "GÜÇ YÖNETİMİ"},
    {"PROBE CONTROL DEFAULTS", "PROB VARSAYILANLARI"},
    {"PROBE RESULT", "PROB SONUCU"},
    {"Password", "Parola"},
    {"Pause", "Duraklat"},
    {"Please edit the machine configuration and set the", "Lütfen makine ayarlarını düzenleyip"},
    {"Please edit the machine configuration and set the\n", "Lütfen makine ayarlarını düzenleyip\n"},
    {"Please insert SD card.", "Lütfen SD kart takın."},
    {"Please wait while the connection is established.", "Bağlantı kurulurken lütfen bekleyin."},
    {"Please wait.", "Lütfen bekleyin."},
    {"Port", "Port"},
    {"Port must be between 1 and 65535.", "Port 1 ile 65535 arasında olmalıdır."},
    {"Position entry requires a connected, IDLE machine.", "Konum girmek için makine bağlı ve BOŞTA olmalı."},
    {"Power", "Güç"},
    {"Powering off...", "Uykuya geçiliyor..."},
    {"Press Scan", "Tara'ya dokunun"},
    {"Prev", "Önceki"},
    {"Probe", "Prob"},
    {"Probe Thickness (mm):", "Prob Kalınlığı (mm):"},
    {"Probe Thickness:", "Prob Kalınlığı:"},
    {"Probing %c%c...", "Prob %c%c..."},
    {"Put the display to sleep?", "Ekran uyutulsun mu?"},
    {"RAPID", "HIZLI"},
    {"RUN", "ÇALIŞIYOR"},
    {"Refresh", "Yenile"},
    {"Requires restart.", "Yeniden başlatma gerekir."},
    {"Reset Defaults", "Varsayılana Dön"},
    {"Reset to defaults", "Varsayılanlara dönüldü"},
    {"Reset to defaults (not saved)", "Varsayılanlar yüklendi (kaydedilmedi)"},
    {"Restart", "Yeniden Başlat"},
    {"Restart Required", "Yeniden Başlatılmalı"},
    {"Restart now?", "Şimdi yeniden başlatılsın mı?"},
    {"Restart returns to machine selection.", "Yeniden başlatma makine seçimine döndürür."},
    {"Restart to change machines,\nor power off for battery operation.\n\nNote: Press reset button to power on\nafter using power off.", "Makine değiştirmek için yeniden başlatın.\nUyandırmak için ekrana 3 saniye basılı tutun."},
    {"Restart to change machines.", "Makine değiştirmek için yeniden başlatın."},
    {"Restarting...", "Yeniden başlatılıyor..."},
    {"Resume", "Sürdür"},
    {"Retract (mm):", "Geri Çekme (mm):"},
    {"Retract:", "Geri Çekme:"},
    {"Retry", "Tekrar Dene"},
    {"Returning to machine selection.", "Makine seçimine dönülüyor."},
    {"Rotate 180° for inverted mounting.", "Ters montaj için 180° döndürür."},
    {"Rotate 180° for inverted mounting.\nRequires restart.", "Ters montaj için 180° döndürür.\nYeniden başlatma gerekir."},
    {"Rotate 180°:", "180° Döndür:"},
    {"Rotate display 180° for", "Ters montaj için ekranı"},
    {"Rotate display 180° for\nupside-down mounting.\nRequires restart.", "Ters montaj için ekranı\n180° döndürür.\nYeniden başlatma gerekir."},
    {"Running: %s", "Çalışıyor: %s"},
    {"Running: Macro Name", "Çalışıyor: Makro Adı"},
    {"SD card not available", "SD kart kullanılamıyor"},
    {"SD card not available.", "SD kart kullanılamıyor."},
    {"SD card not available.\nPlease insert SD card.", "SD kart kullanılamıyor.\nLütfen SD kart takın."},
    {"SELECT WORK COORDINATE SYSTEM", "İŞ KOORDİNAT SİSTEMİNİ SEÇİN"},
    {"SLEEP", "UYKU"},
    {"SPINDLE", "İŞ MİLİ"},
    {"STATE", "DURUM"},
    {"STOP", "DUR"},
    {"SUCCESS", "BAŞARILI"},
    {"SUCCESS\n%s: %.3f mm", "BAŞARILI\n%s: %.3f mm"},
    {"SUCCESS\nX:%.3f Y:%.3f Z:%.3f", "BAŞARILI\nX:%.3f Y:%.3f Z:%.3f"},
    {"Save", "Kaydet"},
    {"Save Changes", "Değişiklikleri Kaydet"},
    {"Save Machine", "Makineyi Kaydet"},
    {"Save Settings", "Ayarları Kaydet"},
    {"Save failed. Please try again.", "Kaydedilemedi. Tekrar deneyin."},
    {"Scan", "Tara"},
    {"Scan failed", "Tarama başarısız"},
    {"Scanning...", "Taranıyor..."},
    {"Screenshot Server", "Ekran Görüntüsü"},
    {"Select Machine", "Makine Seç"},
    {"Select a network or type its SSID.", "Bir ağ seçin veya SSID'sini yazın."},
    {"Selected machine", "Seçili makine"},
    {"Send", "Gönder"},
    {"Server error", "Sunucu hatası"},
    {"Set", "Ayarla"},
    {"Settings", "Ayarlar"},
    {"Settings have been exported to:", "Ayarlar dışa aktarıldı:"},
    {"Settings have been exported to:\n/fluidtouch_settings.json\n\nFile location: Display SD card root", "Ayarlar dışa aktarıldı:\n/fluidtouch_settings.json\n\nKonum: Ekranın SD kartı ana dizini"},
    {"Settings saved!", "Ayarlar kaydedildi!"},
    {"Settings saved! Restart to apply.", "Kaydedildi! Uygulamak için yeniden başlatın."},
    {"Shift", "Büyük"},
    {"Show:", "Göster:"},
    {"Size: %.2f KB", "Boyut: %.2f KB"},
    {"Size: %.2f MB", "Boyut: %.2f MB"},
    {"Sleep", "Uyut"},
    {"Sleep After:", "Ekran:"},
    {"Soft Reset", "Yazılımı Sıfırla"},
    {"Space", "Boşluk"},
    {"Standby:", "Bekleme:"},
    {"Starting...", "Başlatılıyor..."},
    {"Status", "Durum"},
    {"Step", "Adım"},
    {"Step List", "Adım Listesi"},
    {"Step list cannot be empty", "Adım listesi boş olamaz"},
    {"System Options", "Sistem Seçenekleri"},
    {"TOOL", "TAKIM"},
    {"Terminal", "Terminal"},
    {"The device will restart after clearing.", "Ayarlar silindikten sonra cihaz yeniden başlatılır."},
    {"The following items are saved to the backup file:", "Yedek dosyasına kaydedilenler:"},
    {"The following items are saved to the backup file:\n", "Yedek dosyasına kaydedilenler:\n"},
    {"This action cannot be undone.", "Bu işlem geri alınamaz."},
    {"This machine requires a WiFi password", "Bu makine için WiFi parolası gerekli"},
    {"This machine requires a WiFi password\n", "Bu makine için WiFi parolası gerekli\n"},
    {"This saved profile will be permanently removed.", "Bu kayıtlı profil kalıcı olarak silinecek."},
    {"This will permanently delete:", "Kalıcı olarak silinecekler:"},
    {"Transferring file. Please wait.", "Dosya aktarılıyor. Lütfen bekleyin."},
    {"UNITS", "BİRİM"},
    {"UNKNOWN", "BİLİNMİYOR"},
    {"UPLOAD COMPLETE", "YÜKLEME TAMAMLANDI"},
    {"URL: %s:%d", "Adres: %s:%d"},
    {"Unlock", "Kilidi Ağ"},
    {"Upload", "Yükle"},
    {"Upload cancelled", "Yükleme iptal edildi"},
    {"Upload failed", "Yükleme başarısız"},
    {"Upload successful", "Yükleme başarılı"},
    {"Upload to FluidNC", "FluidNC'ye Yükle"},
    {"Uploading...", "Yükleniyor..."},
    {"Use Edit to add macros.", "Makro eklemek için Düzenle'ye dokunun."},
    {"Values must be 0.01-10000", "Değerler 0.01-10000 arasında olmalı"},
    {"Version:", "Sürüm:"},
    {"Version: ", "Sürüm: "},
    {"Version: %s", "Sürüm: %s"},
    {"WARN", "UYARI"},
    {"WCS settings saved", "WCS ayarları kaydedildi"},
    {"WORK COORDINATE SYSTEMS", "İŞ KOORDİNAT SİSTEMLERİ"},
    {"WORK POSITION", "İŞ KONUMU"},
    {"Waiting for a response from FluidNC.", "FluidNC yanıtı bekleniyor."},
    {"Weak", "Zayıf"},
    {"When disabled, the first configured machine", "Kapalıyken başlangıçta ilk kayıtlı"},
    {"When disabled, the first configured machine\nwill be loaded automatically at startup.", "Kapalıyken başlangıçta ilk kayıtlı\nmakine otomatik yüklenir."},
    {"When enabled, folders appear at the top", "Açıkken klasörler dosya listesinin"},
    {"When enabled, folders appear at the top\nof the file list instead of the bottom.", "Açıkken klasörler dosya listesinin\naltı yerine üstünde gösterilir."},
    {"WiFi Connection Failed", "WiFi Bağlantısı Başarısız"},
    {"WiFi Password Required", "WiFi Parolası Gerekli"},
    {"WiFi and machine are disconnected.", "WiFi ve makine bağlı değil."},
    {"WiFi and machine are disconnected.\n\n%s\n\nClick Connect to reconnect.", "WiFi ve makine bağlı değil.\n\n%s\n\nBağlan'a dokunun."},
    {"WiFi is disconnected.", "WiFi bağlı değil."},
    {"WiFi is disconnected.\n\n%s\n\nClick Connect to reconnect.", "WiFi bağlı değil.\n\n%s\n\nYeniden bağlanmak için Bağlan'a dokunun."},
    {"WiFi not", "WiFi"},
    {"WiFi not\nconnected", "WiFi\nbağlı değil"},
    {"WiFi password before attempting to connect.", "bağlanmadan önce WiFi parolasını girin."},
    {"WiFi passwords are NOT included", "Güvenlik nedeniyle WiFi parolaları"},
    {"WiFi passwords are NOT included\nin the backup file for security reasons.", "Güvenlik nedeniyle WiFi parolaları\nyedek dosyasına DAHİL EDİLMEZ."},
    {"Wired", "Kablolu"},
    {"Wireless", "Kablosuz"},
    {"X JOG", "X HAREKET"},
    {"X-AXIS", "X EKSENİ"},
    {"X:%.3f Y:%.3f Z:%.3f", "X:%.3f Y:%.3f Z:%.3f"},
    {"XY Feed:", "XY Hız:"},
    {"XY JOG", "XY HAREKET"},
    {"XY Step", "XY Adım"},
    {"XY Steps: %s", "XY Adımlar: %s"},
    {"Y JOG", "Y HAREKET"},
    {"Y-AXIS", "Y EKSENİ"},
    {"Z Feed:", "Z Hız:"},
    {"Z JOG", "Z HAREKET"},
    {"Z Step", "Z Adım"},
    {"Z Steps: %s", "Z Adımlar: %s"},
    {"Z-AXIS", "Z EKSENİ"},
    {"ZERO AXIS", "EKSENİ SIFIRLA"},
    {"and network connection is available.", "ağ bağlantısından emin olun."},
    {"but none is configured.", "ancak parola tanımlanmamış."},
    {"but none is configured.\n\n", "ancak parola tanımlanmamış.\n\n"},
    {"connected", "bağlı değil"},
    {"hr:min", "sa:dk"},
    {"in the backup file for security reasons.", "yedek dosyasına DAHİL EDİLMEZ."},
    {"min:sec", "dk:sn"},
    {"of the file list instead of the bottom.", "altı yerine üstünde gösterilir."},
    {"seconds", "saniye"},
    {"to take effect.", "yeniden başlatma gerekiyor."},
    {"upside-down mounting.", "180° döndürür."},
    {"will be loaded automatically at startup.", "makine otomatik yüklenir."},
    {"• All machine configurations", "• Tüm makine ayarları"},
    {"• All machine configurations\n", "• Tüm makine ayarları\n"},
    {"• All macros", "• Tüm makrolar"},
    {"• All macros\n", "• Tüm makrolar\n"},
    {"• Jog and probe settings", "• Hareket ve prob ayarları"},
    {"• Jog and probe settings\n", "• Hareket ve prob ayarları\n"},
    {"• Jog settings (XY/Z feed rates)", "• Hareket ayarları (XY/Z hızları)"},
    {"• Jog settings (XY/Z feed rates)\n", "• Hareket ayarları (XY/Z hızları)\n"},
    {"• Machine configurations (name, connection, hostname/IP, port)", "• Makine ayarları (ad, bağlantı, adres/IP, port)"},
    {"• Machine configurations (name, connection, hostname/IP, port)\n", "• Makine ayarları (ad, bağlantı, adres/IP, port)\n"},
    {"• Macros", "• Makrolar"},
    {"• Macros\n", "• Makrolar\n"},
    {"• Power management settings", "• GÜÇ yönetimi ayarları"},
    {"• Power management settings\n", "• GÜÇ yönetimi ayarları\n"},
    {"• Probe settings (feed rate, distances, thickness)", "• Prob ayarları (hız, mesafe, kalınlık)"},
    {"• Probe settings (feed rate, distances, thickness)\n", "• Prob ayarları (hız, mesafe, kalınlık)\n"},
    {"• UI preferences", "• Arayüz tercihleri"},
    {"• WiFi credentials", "• WiFi kimlik bilgileri"},
    {"• WiFi credentials\n", "• WiFi kimlik bilgileri\n"},
    {"• WiFi network names (passwords NOT included)", "• WiFi ağ adları (parolalar DAHİL DEĞİL)"},
    {"• WiFi network names (passwords NOT included)\n", "• WiFi ağ adları (parolalar DAHİL DEĞİL)\n"},
};
}

void UILanguage::init() {
    Preferences prefs;
    prefs.begin(PREFS_SYSTEM_NAMESPACE, true);
    language = prefs.getUChar("ui_lang", 0) == 1 ? Language::Turkish : Language::English;
    prefs.end();
}
UILanguage::Language UILanguage::current() { return language; }
const char *UILanguage::translate(const char *text) {
    if (!text || language == Language::English) return text;
    size_t lo = 0, hi = sizeof(catalog) / sizeof(catalog[0]);
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        int order = strcmp(text, catalog[mid].en);
        if (!order) return catalog[mid].tr;
        if (order < 0) hi = mid; else lo = mid + 1;
    }
    return text;
}
void UILanguage::render(const char *text, char *out, size_t capacity) {
    if (!capacity) return;
    if (!text) { *out = 0; return; }
    const char *direct = translate(text);
    if (direct != text || language == Language::English) { snprintf(out, capacity, "%s", direct); return; }
    // LVGL symbols prefix many labels. Keep icons and translate the phrase.
    const char *phrase = text;
    while (*phrase == ' ' || *phrase == '\n' || (unsigned char)*phrase == 0xef) {
        phrase += (unsigned char)*phrase == 0xef && phrase[1] && phrase[2] ? 3 : 1;
    }
    direct = translate(phrase);
    if (direct != phrase) { snprintf(out, capacity, "%.*s%s", (int)(phrase-text), text, direct); return; }
    if (strncmp(text, "Version: ", 9) == 0) {
        snprintf(out, capacity, "%s%s", translate("Version: "), text + 9); return;
    }
    // Composite bullet lists and title+subtitle labels are translated by line.
    size_t used = 0;
    while (*text && used + 1 < capacity) {
        const char *end = strchr(text, '\n');
        size_t length = end ? (size_t)(end-text) : strlen(text);
        char line[1024];
        size_t copied = std::min(length, sizeof(line)-1);
        memcpy(line, text, copied); line[copied] = 0;
        const char *translated = translate(line);
        size_t count = std::min(strlen(translated), capacity-used-1);
        memcpy(out+used, translated, count); used += count;
        if (!end) break;
        if (used+1 < capacity) out[used++] = '\n';
        text = end+1;
    }
    out[used] = 0;
}
void UILanguage::setText(lv_obj_t *obj, const char *text) {
    if (!text) { lv_label_set_text(obj, text); return; }
    if (language == Language::English) {
        if (strcmp(lv_label_get_text(obj), text)) lv_label_set_text(obj, text);
        return;
    }
    // The common case needs no scratch buffer; unchanged status labels also
    // avoid an LVGL allocation and redraw on each update.
    const char *direct = translate(text);
    if (direct != text) {
        if (strcmp(lv_label_get_text(obj), direct)) lv_label_set_text(obj, direct);
        return;
    }
    char localized[2048]; render(text, localized, sizeof(localized));
    if (strcmp(lv_label_get_text(obj), localized)) lv_label_set_text(obj, localized);
}
void UILanguage::setTextFmt(lv_obj_t *obj, const char *fmt, ...) {
    char localized[1024]; render(fmt, localized, sizeof(localized));
    va_list args; va_start(args, fmt); lv_label_set_text_vfmt(obj, localized, args); va_end(args);
}
int UILanguage::format(char *dst, size_t size, const char *fmt, ...) {
    char localized[1024]; render(fmt, localized, sizeof(localized));
    va_list args; va_start(args, fmt); int result = vsnprintf(dst, size, localized, args); va_end(args); return result;
}
void UILanguage::setPlaceholder(lv_obj_t *obj, const char *text) { lv_textarea_set_placeholder_text(obj, translate(text)); }
void UILanguage::setOptions(lv_obj_t *obj, const char *text) { lv_dropdown_set_options(obj, translate(text)); }
lv_obj_t *UILanguage::addTab(lv_obj_t *obj, const char *text) { return lv_tabview_add_tab(obj, translate(text)); }

extern "C" {
LV_FONT_DECLARE(tr_12);
LV_FONT_DECLARE(tr_14);
LV_FONT_DECLARE(tr_16);
LV_FONT_DECLARE(tr_18);
LV_FONT_DECLARE(tr_20);
LV_FONT_DECLARE(tr_22);
LV_FONT_DECLARE(tr_24);
LV_FONT_DECLARE(tr_26);
LV_FONT_DECLARE(tr_28);
LV_FONT_DECLARE(tr_32);
LV_FONT_DECLARE(tr_48);
}
const lv_font_t *UILanguage::font(const lv_font_t *base) {
    if (base == &lv_font_montserrat_12) {
        static lv_font_t copy = lv_font_montserrat_12;
        copy.fallback = &tr_12; return &copy;
    }
    if (base == &lv_font_montserrat_14) {
        static lv_font_t copy = lv_font_montserrat_14;
        copy.fallback = &tr_14; return &copy;
    }
    if (base == &lv_font_montserrat_16) {
        static lv_font_t copy = lv_font_montserrat_16;
        copy.fallback = &tr_16; return &copy;
    }
    if (base == &lv_font_montserrat_18) {
        static lv_font_t copy = lv_font_montserrat_18;
        copy.fallback = &tr_18; return &copy;
    }
    if (base == &lv_font_montserrat_20) {
        static lv_font_t copy = lv_font_montserrat_20;
        copy.fallback = &tr_20; return &copy;
    }
    if (base == &lv_font_montserrat_22) {
        static lv_font_t copy = lv_font_montserrat_22;
        copy.fallback = &tr_22; return &copy;
    }
    if (base == &lv_font_montserrat_24) {
        static lv_font_t copy = lv_font_montserrat_24;
        copy.fallback = &tr_24; return &copy;
    }
    if (base == &lv_font_montserrat_26) {
        static lv_font_t copy = lv_font_montserrat_26;
        copy.fallback = &tr_26; return &copy;
    }
#if LV_FONT_MONTSERRAT_28
    if (base == &lv_font_montserrat_28) {
        static lv_font_t copy = lv_font_montserrat_28;
        copy.fallback = &tr_28; return &copy;
    }
#endif
    if (base == &lv_font_montserrat_32) {
        static lv_font_t copy = lv_font_montserrat_32;
        copy.fallback = &tr_32; return &copy;
    }
#if LV_FONT_MONTSERRAT_48
    if (base == &lv_font_montserrat_48) {
        static lv_font_t copy = lv_font_montserrat_48;
        copy.fallback = &tr_48; return &copy;
    }
#endif
    return base;
}
void UILanguage::setFont(lv_obj_t *obj, const lv_font_t *base, lv_style_selector_t selector) { lv_obj_set_style_text_font(obj, font(base), selector); }
