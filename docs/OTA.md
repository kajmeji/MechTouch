# MechTouch GitHub OTA

Panel, Wi-Fi bağlantısından sonra bir kez GitHub Releases üzerinde yeni sürüm arar.
Elle kontrol: **Settings > About > Check for updates**. İnternet erişimi ve saat
eşitlemesi (NTP) gerekir; yalnızca FluidNC erişim noktasına bağlanmak internet sağlamaz.

Yeni sürüm penceresinde **Later / Close** erteler, **Install & Restart** kurulumu başlatır.
Kurulum için FluidNC bağlantısı, son 3 saniye içinde alınmış Idle raporu,
durmuş spindle ve bitmiş makro/dosya aktarımı gerekir. İndirme sırasında hareket,
bağlantı kaybı veya eski durum raporu görülürse işlem iptal edilir. Bu kontrol
makinenin fiziksel acil durdurma sisteminin yerini almaz; diğer kontrol istemcilerinden
iş başlatılmamalıdır.

## İlk USB geçişi

Derleme platformu TLS içeren pioarduino **55.03.37 / Arduino 3.3.7** sürümüne
sabitlenmiştir. Eski Tasmota paketinde TLS uygulaması bulunmadığından bu geçiş
gereklidir; USB kurulumundan sonra ekran, dokunmatik ve makine bağlantısı da denenmelidir.

1. Ayarları panelin yedekleme ekranından dışa aktarın.
2. Basic için `pio run -e elecrow-crowpanel-7-basic -t upload` ile tam USB yüklemesi yapın.
   Sadece `firmware.bin` dosyasını eski bölüm tablosuna yazmak yeterli değildir.
3. Seri monitörde `[OTA-CHECK]` satırında `next=2031616 otadata=1` görülmelidir.
4. İnternet erişimi olan Wi-Fi ağına bağlanın, makine Idle iken güncellemeyi kontrol edin.

Yerel Basic derlemesinden USB ZIP paketi üretmek için
`python scripts/prepare_usb_release.py` çalıştırılabilir. Çıktı `dist/usb/` içindedir.
Paket, NVS alanının üzerinden geçen birleşik bir imaj içermez; dört dosya kendi
adreslerine yazılır. Adresler, SHA-256 değerleri ve yükleme komutu paketin içindedir.
OTA için yalnızca `dist/ota/` altındaki uygulama dosyası ve manifest kullanılır.

Basic düzeni: NVS `0x9000/0x5000`, OTA metadata `0xe000/0x2000`,
iki uygulama alanı `0x10000` ve `0x200000` başlangıçlarında 1.9375 MiB,
coredump `0x3f0000/0x10000`. Basic için dahili LittleFS bölümü yoktur; mevcut
uygulama dosyaları SD kartta, ayarları NVS içinde tutar.
NVS konumu korunur; eski dahili LittleFS içeriği korunmaz.
SD kart bu değişiklikten etkilenmez.

25 Ekim 2026 yerel Basic 1.0.5 derlemesi: firmware **2.008.112 bayt**,
her OTA alanında **23.504 bayt** boşluk. Büyüme payı sınırlıdır; paketleyici her
sürümde gerçek uygulama dosyasının alana sığdığını denetler. Yerel Basic derlemesi,
10 paketleme testi, sürüm karşılaştırma testi, görsel/font çözme testleri geçti.
Advance modelleri için derleme adımları CI'da bulunur; bu yerel doğrulama Basic içindir.

## Sürüm yayımlama

`kajmeji/MechTouch` deposu ve Release dosyaları kimlik doğrulama gerektirmeden
erişilebilir olmalıdır. Firmware içine GitHub erişim tokenı konmaz.
Kaynak değişikliklerini GitHub'a gönderdikten sonra `v1.0.6` gibi bir **vX.Y.Z** etiketi
yayımlandığında mevcut Build and Deploy Firmware iş akışı üç modeli derler,
sürüm numaralarını günceller ve OTA dosyalarını Release'e ekler.
Ön sürümler desteklenmez; aynı veya daha eski sürüm kurulmaz.

Her model için iki dosya gerekir:

- `ota-basic.json` (Advance için `ota-advance-v1.2.json` / `ota-advance-v1.3.json`)
- `mechtouch-basic-1.0.6.bin` (model ve sürüme göre adlandırılır)

Yerel paketleme örneği (derleme ve config sürümü eşleşmelidir):

```powershell
python scripts/prepare_ota_release.py 1.0.5 --board basic
```

Çıktı `dist/ota/` klasöründedir. Paketleyici ESP32-S3 uygulama başlığını, firmware
içindeki sürümü, OTA bölümlerini ve boyutu denetler. OTA'ya bootloader, partitions
veya birleştirilmiş flash dosyası verilmez.

TLS sertifikaları `include/network/ota_ca.h` içindeki 11 Mozilla kök sertifikasıyla
doğrulanır. Bu liste GitHub ve Release sunucuları için sınırlandırılmıştır; CA
değişikliklerinde `scripts/update_ota_ca.py` ile gözden geçirilip yenilenmelidir.
Betik, dosyayı değiştirmeden önce üç sunucuda TLS 1.2 ve güncel TLS sürümüyle
sertifika zinciri ve alan adı doğrulaması yapar. Yönlendirmeler yalnızca GitHub'ın
HTTPS Release/asset adreslerine yapılır. Manifest model, sürüm, boyut ve SHA-256
içerir. İndirme tamamlanıp SHA-256 ve ESP uygulama doğrulaması geçmeden boot bölümü
değiştirilmez. SHA-256 ayrı bir imza sistemi değildir; güven kaynağı HTTPS ve GitHub
Release hesabıdır. Yeni firmware açıldıktan sonra otomatik sağlık kontrolü/rollback
bu sürümde uygulanmamıştır.

## Donanım üzerinde kabul kontrolü

- Aynı sürüm: güncel mesajı; daha yeni sürüm: onay penceresi.
- İnternetsiz ağ / erişilemeyen Release: anlaşılır hata, çalışan firmware korunur.
- Run/Hold/Jog veya bağlantısız makine: kurulum düğmesi kapalı.
- İndirmede bağlantı kesilmesi veya makinenin hareket etmesi: iptal, yeniden başlatma yok.
- Yanlış model/boyut/SHA-256: reddetme, çalışan firmware korunur.
- Başarılı kurulum: yeni sürümle açılış, ayarların ve makine bağlantısının kontrolü.
- Sonraki sürümle ikinci OTA: diğer uygulama alanına geçişin kontrolü.

Yerel derleme/paketleme testleri gerçek panelde indirme ve yeniden başlatma testinin
yerini tutmaz. İlk kurulumdan önce yukarıdaki akış fiziksel panelde doğrulanmalıdır.

## Flash boyutu optimizasyonu

Montserrat fontları LVGL 9.5.0 kaynaklarından kayıpsız sıkıştırılmıştır;
`src/ui/fonts/ota_montserrat_*.c` dosyaları derlemeye dahildir. Dahili ham kopyalar
`lv_conf.h` içinde kapalıdır. Türkçe fallback bağlantıları korunur. Font lisansları
`src/ui/fonts/licenses/` içindedir. Üretim için `scripts/compress_fonts.cjs` ve
`lv_font_conv@1.5.3` kullanılır; normal firmware derlemesi Node gerektirmez.

CI, 1.404 karakterin ölçülerini karşılaştırır ve LVGL'nin gerçek çözücüsüyle bütün
piksellerin aynı kaldığını denetler. Açılış görseli de aynı yöntemle kayıpsız RLE
sıkıştırılmıştır. Kullanılmayan hedef çizim biçimleri kapatılmış; RGB565 ekran,
alfa katmanları ve tek renkli çizim desteği korunmuştur.
