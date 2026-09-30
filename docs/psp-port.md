# KeeperFX — Sony PSP portu

Bu belge KeeperFX'in PSP'ye nasıl port edildiğini, yol boyunca çıkan sorunları
ve her birinin nasıl teşhis edilip çözüldüğünü anlatır. Kod `psp-port` dalında,
`3ca5921` (upstream master) üzerine yapılmış commit'lerdir.

- Hedef: PSP-2000 / 3000 / Go, özel yazılım (CFW: PRO, LME, ARK, Infinity) ile.
  PSP-1000 desteklenmez (bellek yetmez).
- Durum: Gerçek PSP'de açılış videosu, ana menü, kampanya haritası ve seviye 1
  çalışıyor. Bellek ve hız hâlâ sınırda; aşağıdaki "Açık konular"a bakın.

---

## 1. Derleme

Gereken: pspdev (`~/pspdev`, psp-gcc 15, SDL3, SDL3_mixer, OpenAL, Lua 5.4 içerir).

```sh
export PSPDEV=~/pspdev PATH=~/pspdev/bin:$PATH
psp-cmake -S . -B out-psp -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C out-psp keeperfx          # -> out-psp/EBOOT.PBP
```

Bütün PSP'ye özel derleme mantığı `build/cmake/modules/PSP.cmake` içindedir
(`CMakeLists.txt`, `PSP` tanımlıysa yalnızca bu modülü kullanır).

| CMake seçeneği | Varsayılan | Ne işe yarar |
|---|---|---|
| `KFX_PSP_MEMDEBUG` | OFF | Büyük `malloc`'ları ve ilk bellek hatasında canlı heap dökümünü `memdebug.log`'a yazar |
| `KFX_PSP_PERFLOG` | OFF | Her 5 saniyede FPS'i `keeperfx.log`'a yazar |
| `KFX_PSP_TRACE` | OFF | Açılış adımlarını (her ayar satırı dahil) `psp_trace.txt`'ye yazar, her satırdan sonra dosyayı kapatır |
| `KFX_PSP_ENC_PRX` | OFF | PRX'i imzalar; KeeperFX için çalışmaz (bkz. Sorun 12) |

PSP derlemesinin kaynakta açtığı tanımlar:

| Tanım | Anlamı |
|---|---|
| `KFX_PSP` | PSP'ye özel kod yolları |
| `KFX_NO_OPENGL`, `KFX_NO_NETWORK` | OpenGL ve ağ kodu derlenmez / stub'lanır |
| `KFX_FMV_SMACKER` | Videolar FFmpeg yerine libsmacker ile |
| `KFX_LAZY_SOUND_BANKS` | Ses bankası örnekleri tembel yüklenir |
| `KFX_GAME_ON_HEAP` | `struct Game` açılışta heap'ten ayrılır |
| `KFX_NATURAL_ALIGNMENT` | Başlıklardaki `#pragma pack(1)` devre dışı |
| `KFX_SPRITE_CACHE` | Çözülmüş sprite önbelleği (`*.kfxc`) |
| `KFX_KEEPSPRITE_BUDGET` | Yaratık sprite havuzu üst sınırı (3 MB) |

Dışarıdan eklenen kütüphaneler `deps/psp/` altında: libspng, centijson,
astronomy, zlib contrib/minizip (klasik `unzip.h` API'si), libsmacker (LGPL-2.1).

## 2. Paket ve kurulum

```sh
psp/make_psp_package.sh <dk_dir> <kfx_release_dir> out-psp/EBOOT.PBP <cikis_klasoru>
```

- `<dk_dir>`: Orijinal Dungeon Keeper dosyaları. GOG kurulum dosyası Linux'ta
  `innoextract` ile açılabilir (`DATA/`, `LDATA/`, `SOUND/`, `keeper0*.ogg`).
- `<kfx_release_dir>`: KeeperFX "complete" sürüm arşivi (ör. 1.4.0).
- Betik, sürüm arşivinin üzerine bu kaynak ağacının `config/fxdata`,
  `config/creatrs`, `campgns`, `levels` klasörlerini ve `make pkg-languages`
  ile üretilen dil dosyalarını kopyalar (bkz. Sorun 8), DK dosyalarını küçük
  harfle ve üzerine yazmadan ekler, yalnızca harf büyüklüğü farklı yinelenen
  dosyaları temizler (bkz. Sorun 17).

Çıkan klasör PSP'de `ms0:/PSP/GAME/KeeperFX/` olarak kopyalanır (~650 MB).
`EBOOT.PBP` klasörün doğrudan içinde olmalıdır.

EBOOT'un yanına konabilecek dosyalar:

- `keeperfx_args.txt`: Komut satırı argümanları (ör. `-level 1`). XMB argüman geçmediği için.
- `keeperfx.cfg`: PSP ayarları (`psp/keeperfx.cfg`; 480x272, yazılım renderer).

Oyunun yazdığı teşhis dosyaları: `keeperfx.log`, `psp_boot.txt`, isteğe bağlı
`psp_trace.txt` / `memdebug.log`.

## 3. Kontroller

| Tuş | İşlev |
|---|---|
| Analog | İmleç |
| × | Seç (sol tık) |
| △ | Geri: oyunda sağ tık (bırak/iptal), menülerde Escape |
| ○ / □ | Yaklaş / uzaklaş |
| L / R | Kamerayı döndür (yaratığa girilmişken yetenek değiştir) |
| D-pad | Haritayı kaydır |
| Start / Select | Menü / harita |

Tanımlar `src/front_input.c` içinde `PSPBTN(masaüstü, psp)` makrosuyla yapılır.

---

## 4. Karşılaşılan sorunlar ve çözümleri

Her madde: **belirti → kök neden → çözüm**. Sıra, karşılaşılma sırasıdır.

### Sorun 1 — İlk derleme hataları

- `int32_t` PSP'de (newlib/MIPS) `long`, x86'da `int`; `int*`/`int32_t*` karışımları derlenmedi.
  → İlgili türler eşitlendi (`TextCommands.h`, `light_data.c`, `main.cpp`).
- pspdev'de Lua 5.4 var, KeeperFX LuaJIT (5.1 API) kullanıyor.
  → `src/psp/lua.h` ve `lauxlib.h` sarmalayıcıları eksik isimleri
  (`lua_objlen`, `luaL_checkint`, …) geri ekler.
- pspdev'deki minizip-ng'de `unzip.h` yok → zlib'in klasik minizip'i eklendi.
- `SDL3_mixer` da `dr_mp3`'ü dışa açık sembollerle içeriyordu, link çakıştı.
  → PSP'de KeeperFX'in kopyası `static` yapıldı (`DRMP3_API static`).
- OpenGL, enet/curl/upnp (ağ), FFmpeg (video) PSP'de yok.
  → Hariç tutuldu; ağ fonksiyonları `src/psp/net_stub.c` ile "ağ yok" döndürür.
- Yeni platform sınıfı `PlatformPSP` (`src/kfx/platform/PlatformPSP.*`), Linux sınıfını genişletir.

### Sorun 2 — Statik bellek 152 MB

- Belirti: ELF'in `.bss` bölümü 152 MB; PSP-2000'de kullanıcı belleği ~52 MB.
- Kök neden: KeeperFX modlama için çok büyük sınırlar kullanıyor: 170x170 harita,
  12288 nesne, 1024 yaratık, 32 doku seti, 16 MB çokgen havuzu vb.
- Çözüm (yalnızca PSP): 85x85 harita (orijinal DK boyutu), 2048+1024 nesne,
  256 yaratık, 1 doku seti, 1 MB çokgen havuzu, küçültülmüş tamponlar
  → 34.7 MB. Doku seti taşması `engine_remap_texture_blocks`'ta kırpılıyor.

### Sorun 3 — Ses bankası belleğe sığmadı

- Belirti: `OpenAL: Cannot buffer sample data: Out of memory`.
- Kök neden: `sound.dat` (43 MB) açılışta tamamen OpenAL'a yükleniyor.
- Çözüm: `KFX_LAZY_SOUND_BANKS`. Açılışta sadece dizin okunur, örnekler ilk
  çalındıklarında yüklenir; bütçe aşılınca en eski kullanılan atılır (LRU,
  şu an 1.5 MB). Çalmakta olan örnek atılmaz.

### Sorun 4 — Müzik ve bellek tüketen diğer yüklemeler

- Müzik: `MIX_LoadAudio` OGG'nin tamamını RAM'e alıyordu (~5 MB)
  → PSP'de `MIX_SetTrackIOStream` ile dosyadan akış.
- Özel sprite'lar en kötü durum boyutunda ayrılıyordu
  → RLE sıkıştırmadan sonra gerçek boyuta küçültülüyor (`compress_raw` artık boyut döndürür).
- `calculate_file_checksum` dosyayı tamamen okuyordu
  → paketsiz dosyalar 64 KB'lık parçalarla (`rnc_crc_update`).
- Teşhis: `KFX_PSP_MEMDEBUG` (`--wrap=malloc/calloc/realloc/free`) ile canlı
  heap arayanlara göre gruplanıp döküldü.

### Sorun 5 — Menüde yazı yok

- Belirti: Ana menü butonları çiziliyor ama üzerlerinde metin yok.
- Kök neden: Menü fontları (`LoadVResMinimal`) sadece ekran yüksekliği ≥ 400
  iken yükleniyordu; 480x272'de `frontend_font[]` NULL kalıyordu.
- Çözüm: Menü verisi her çözünürlükte yükleniyor; serbest bırakma gerçekten
  yüklenip yüklenmediğine göre yapılıyor (`vidmode.c`).

### Sorun 6 — Emülatörde test altyapısı

- Headless PPSSPP ekran görüntüsü veremiyor (`Could not download output`).
  → `psp/tools/ppsspp_drive.py`: websocket debugger ile CPU durdurulup VRAM
  (`0x04000000`, iki 512-satır RGBA tampon) okunarak PNG yazılır; tuş/analog gönderilir.
- Takılı kalan `PPSSPPHeadless` süreçleri debugger portunu tutuyordu; betik
  artık her seferinde boş port seçiyor ve çıkarken emülatörü kapatıyor.

### Sorun 7 — Yükleme ve performans (emülatör)

- CPU varsayılan 222 MHz → `scePowerSetClockFrequency(333,333,166)`; oyun içi ~14 → ~24 FPS.
- Özel sprite zip'leri her seviye başında baştan çözülüyordu → kaynak listesi
  (kampanya klasöründeki zip'ler, seviye zip'i, modlar) değişmediyse yeniden
  yüklenmiyor (sadece PSP).

### Sorun 8 — Veri ve kod sürümü uyumsuzluğu

- Belirti: "Campaigns" butonu boş, `font12.fxfont` bulunamıyor.
- Kök neden: Kod master'dan, veri 1.4.0 sürümünden; dil dosyalarındaki metin
  kimlikleri ve font formatı değişmiş.
- Çözüm: Paket betiği master'ın yapılandırma/kampanya dosyalarını ve
  `make pkg-languages` ile üretilen `gtext_*.dat`'ı kopyalıyor. Unifont
  tabloları (birkaç MB) PSP'de sadece çift baytlı dillerde yükleniyor.

### Sorun 9 — Ara videolar

- FFmpeg yok → `KFX_FMV_SMACKER`: libsmacker ile Smacker akışı diskten okunur,
  kareler mevcut paletli `RendererPresentImage` yoluyla çizilir, ses SDL ses akışıyla çalar.
- Video sonunda donma: Ses kuyruğunun boşalması sınırsız bekleniyordu
  (headless'ta ses tüketilmez) → bekleme kalan ses süresiyle sınırlandı, kuyruk 2 sn ile sınırlandı.
- Tuşla atlanamıyordu: Açılış videosu oynarken tuş ayarları henüz yüklü değil
  → herhangi bir gamepad yüz/omuz tuşu videoyu doğrudan atlar.

### Sorun 10 — Gerçek PSP: "The game could not be started (80020148)" ve isim görünmüyor

- Kök neden: `PARAM.SFO`'da `APP_VER` boş dizgeydi (pspdev'in `CreatePBP`'si
  sürüm verilmezse böyle yazıyor). PPSSPP umursamıyor, XMB kaydı reddediyor.
- Çözüm: `create_pbp_file(... VERSION "01.40")`.

### Sorun 11 — Gerçek PSP: isim geldi ama yine 80020148

- Teşhis: Çalışan bir Godot EBOOT'u ile karşılaştırıldı. Godot 22.3 MB,
  KeeperFX 34.7 MB bellek imgesine sahipti.
- Kök neden: Modül yüklenirken PSP'nin 24 MB'lık kullanıcı bölümüne sığmıyordu
  (genişletilmiş bellek o anda devrede değil).
- Çözüm: `KFX_GAME_ON_HEAP` — 14 MB'lık `struct Game`, bir kurucu
  (`constructor(101)`) içinde `calloc` ile ayrılıyor; kod `game` adını bir
  makro (`#define game (*kfx_game_ptr)`) üzerinden aynen kullanmaya devam ediyor.
  Modül 19.7 MB'a indi.

### Sorun 12 — PRX imzalama denemesi

- Amaç: CFW olmadan da açılabilmesi. pspdev'in `PrxEncrypter`/`ebootsign`
  araçları hazır Sony başlık şablonları kullanıyor; en büyüğü ~5.5 MB, KeeperFX
  ~10 MB. İmzalı sürüm mümkün değil; seçenek varsayılan olarak kapalı bırakıldı.

### Sorun 13 — Gerçek PSP: siyah ekran, bir süre sonra kapanma (bellek)

- Teşhis: `psp_boot.txt` açılışta sistemde sadece ~0.7 MB kaldığını gösterdi.
- Kök neden: `PSP_HEAP_SIZE_KB(-N)` negatif değerin büyüklüğünü kullanmıyor;
  pspsdk "tüm bellek eksi eşik" hesaplıyor ve eşik ayrı makroyla veriliyor
  (varsayılan 512 KB). İş parçacıkları ve çekirdek ayırmaları için yer kalmıyordu.
- Çözüm: `PSP_HEAP_SIZE_KB(-1); PSP_HEAP_THRESHOLD_SIZE_KB(4096);`.
  Heap küçülünce oyun durumu için yapılandırma tabloları da küçültüldü
  (2000'lik sınırlar → 256/512; en yüksek kullanılan indeks 184), `struct Game` 14.7 → 9 MB.

### Sorun 14 — Gerçek PSP: siyah ekran (hizasız bellek erişimi)

- Teşhis: `KFX_PSP_TRACE` iz sürümü, takılmanın `keeperfx.cfg`'deki
  `FRAMES_PER_SECOND` satırında olduğunu gösterdi.
- Kök neden: `start_params` `#pragma pack(1)` ile paketli; `num_fps_draw_main`
  4'ün katı olmayan adreste ve `int32_t*` üzerinden yazılıyor. PSP'nin CPU'su
  hizasız 2/4 baytlık erişimde adres hatası verir; PPSSPP bunu sessizce tolere eder.
- Kapsam: GCC `#pragma pack` yapıları için `-Waddress-of-packed-member` uyarısı
  vermiyor. Kaynağın bir kopyasında paketli yapılara `__attribute__((packed))`
  eklenip derlenince 183 risk noktası çıktı.
- Çözüm:
  - `KFX_NATURAL_ALIGNMENT`: 134 başlıktaki `#pragma pack(1)` PSP'de devre dışı.
    Sadece disk formatı olan `map_columns.h` (`.CLM` kayıtları, 4'e hizalı
    yapıldı) ve `.c/.cpp` içindeki yerel dosya yapıları paketli kaldı. Kayıt ve
    tekrar dosyaları sadece PSP'nin kendi dosyaları, düzen değişebilir.
  - Kalan tekil sorunlar: sprite blit'inde 32-bit kaynak okuması (`memmove`),
    sprite uzunluk okuması (`memcpy`), `big_scratch` içinden türetilen dizilerin
    ofseti (4'e yuvarlandı), sprite dizini ve keşif tablosundaki hizasız
    okumalar, `poly_pool` / `big_scratch` 16 bayta hizalı.
- Doğrulama aracı: `psp/tools/ppsspp-report-unaligned.patch` PPSSPP'nin
  yorumlayıcısına her hizasız `lh/lhu/sh/lw/sw/lwc1/swc1` için PC ve adres
  raporu ekler. `PPSSPPHeadless -i` ile çalıştırılır. Eski sürümde gerçek
  PSP'deki hatayı yakaladı; düzeltilmiş sürümde açılış, menü ve seviye 1
  boyunca sıfır rapor.

### Sorun 15 — `._` dosyaları

- Kopyalama aracı karta 5916 adet "Mac OS X" AppleDouble (`._*`) dosyası
  yazmıştı; bazıları `._keeperfx.cfg`, `._classic.cfg` gibi oyunun tarayacağı
  adlardaydı. Silindi. Karta Mac tabanlı bir araçla kopyalanıyorsa tekrar
  oluşabilir.

### Sorun 16 — Kontroller kararsız

- Tuş düzeni ×/○/□/△'yü "değiştirici" sayıyordu: biri basılıyken diğer atamalar
  tetiklenmiyordu (ör. × basılıyken D-pad ile kaydırma).
- Çözüm: PSP'de değiştirici yok, her işlev tek tuş (bkz. Bölüm 3). Kartta
  kayıtlı eski kontrolcü atamaları PSP'de yok sayılıyor. Zoom out sınırı açıldı
  (`MAX_ZOOM_DISTANCE=30`).

### Sorun 17 — Yinelenen dosya adları

- Sürüm arşivinde `MAP00001.TXT`, kaynakta `map00001.txt` var; FAT ikisini
  tutamadığı için kopyalayıcı birine `(1)` ekledi. Paket betiği kaynak ağacındaki kopyayı tutuyor.

### Sorun 18 — Gerçek PSP'de yükleme çok uzun

- Ölçüm: `keeperfx.log`'daki `PSP load:` satırları her açılış adımının süresini verir.
- Özel sprite'lar (emülatörde 22.5 sn):
  - `KFX_SPRITE_CACHE`: `sprites.json` ve `icons.json` için çözülen her PNG'nin
    RLE sonucu `<zip>.kfxc` / `<zip>.icons.kfxc` dosyasına yazılır, sonraki
    açılışlarda sırayla oynatılır. Zip boyutu ve tarihi değişince önbellek
    kendini yeniler. Önbellekten okunan verinin ve tabloların çözülenle
    aynı olduğu sağlama toplamıyla doğrulandı.
  - Bir zip'in 5 JSON araması için zip ve dizin önbelleği bir kez açılıyor.
  - Sonuç: 22.5 → 3.9 sn (ilk açılışta önbellek yazıldığı için ~18 sn).
- Çok oyunculu harita paketleri ağ yokken listelenmiyor (−2.5 sn).
- Toplam (emülatör, intro sonrası → menü): ~35 → ~14 sn.

### Sorun 19 — Analog imleçle nişan almak zor

- Ölü bölge %30'du ve çıkışta imleç ani hızla başlıyordu.
- Çözüm (PSP): ölü bölge 5500 ve sonrası 0'dan ölçekleniyor; hız 40 → 300 px/sn
  karesel eğri; 0.35 sn tam itişten sonra 600 px/sn'ye kadar hızlanma; GUI
  düğmesi üzerindeyken yarı hız (`kjm_input.c`).

### Sorun 20 — Gerçek PSP'de oyun içinde bellek bitiyor

- Belirti: Kampanya haritasında konuşma MP3'ü ve oyun içinde ses efektleri
  `Out of memory` / `std::bad_alloc`.
- Kök nedenler:
  1. PPSSPP homebrew'a gerçek PSP'den birkaç MB fazla bellek veriyor gibi
     görünüyor; emülatördeki ~6 MB pay gerçek cihazda yoktu.
  2. `creature.jty` kareleri ihtiyaç oldukça `malloc` ediliyor ve **hiç
     serbest bırakılmıyordu** (uzun oyunda 13 MB'a kadar); `reset_heap_manager`
     işaretçileri serbest bırakmadan sıfırlıyordu.
  3. Konuşmalar tamamen PCM'e çözülerek yükleniyordu.
- Çözüm:
  - `KFX_KEEPSPRITE_BUDGET` (3 MB): Her kare sunumundan sonra (hiçbir çizim
    komutu eski veriye işaret etmezken) en uzun süredir çizilmeyen kareler
    atılır. Her çizim yolu zaten önce yükleyiciyi çağırdığı için gerektiğinde
    dosyadan tekrar okunur. 64 KB bütçeyle zorlama testi yapıldı.
  - Konuşmalar dosyadan akıtılıyor; ses bütçesi 1.5 MB.
  - `keeperfx.log`: ana menü, seviye başı ve her 30 sn'de heap kullanımı ve en
    büyük serbest blok (`PSP memory (...)` satırları).

### Sorun 21 — Önbelleğe rağmen gerçek PSP'de sprite yüklemesi 43 sn

- Ölçüm (gerçek PSP, ikinci açılış): özel sprite'lar 43.3 sn, yapılandırma
  15.5 sn, kampanya listeleri 10.9 sn. Önbellek dosyaları kullanılmıştı.
- Kök neden: Önbellekten okurken bile her sprite karesi için zip içinde dosya
  aranıyor/açılıyor/kapatılıyordu; 3420 kare × hafıza kartı erişimi. Emülatör
  hafıza kartı gecikmesini taklit etmediği için burada görünmedi.
- Çözüm: Önbellekten okunurken zip'e hiç dokunulmuyor; önbellek tam o karede
  bozulursa o kare zip'ten çözülüyor. minizip dosya tamponu 32 KB.

### Sorun 22 — Yapılandırma ayrıştırması yavaş

- Kök neden: `find_conf_block` her blok için dosyayı baştan tarıyor
  (N blok → N tam tarama).
- Çözüm (PSP): Tampon başına bir kez oluşturulan blok dizini. 4000'den fazla
  aramada eski taramayla birebir aynı sonuç (konum, satır numarası) doğrulandı.
  Emülatörde yapılandırma süresi açılışta 5.5 → 4.0 sn, seviye başında
  6.3 → 4.8 sn. Kalan süre alan ayrıştırmasında; profil çıkarmak için
  `-pg`/`libpspprof` denendi ama PPSSPP'de örnek toplanmadı.

### Sorun 23 — Tuzak/kapı tablosunda dizi taşması

- Derleyici uyarısı (`-Waggressive-loop-optimizations`) ile bulundu:
  `object_to_door_or_trap[]` nesne sayısı kadar, onu temizleyen döngü tuzak
  sayısı kadar dönüyordu. Masaüstünde ikisi de 2000 olduğu için zararsızdı;
  PSP'de nesne sınırı 512 olunca sonraki yapılandırma verisinin üzerine sıfır
  yazıyordu. Döngü dizinin kendi boyutuyla sınırlandı.

### Not — Yaratık bırakma

- DK'de elindeki yaratığı bırakmak sağ tıktır (PSP'de △). Oda kurma aracı
  seçiliyken (yeşil küp) ilk sağ tık aracı kapatır, ikincisi yaratığı bırakır.

---

## 5. Test yöntemi

- **Emülatör:** `~/ppsspp-build/PPSSPPHeadless <EBOOT> -r <klasör> --timeout=N`.
  Oyun `keeperfx.log`'u klasöre yazar. `keeperfx_args.txt` ile doğrudan seviye açılabilir.
- **Etkileşimli test ve ekran görüntüsü:** `psp/tools/ppsspp_drive.py`
  (`websocket-client` gerekir). Headless genelde gerçek zamandan hızlı koşar,
  bu yüzden zamanlamaya bağlı testler güvenilmez; mümkünse seviye içinde test edilmeli.
- **Hizasız erişim:** Yamalı PPSSPP ile `-i` (yorumlayıcı) modunda çalıştırılıp
  `UNALIGNED ... pc=` satırları `psp-addr2line` ile (PC − `0x08804000`) çözülür.
- **Gerçek PSP:** Kart USB modunda `/media/<kullanıcı>/disk` olarak bağlanır.
  `psp_boot.txt`, `keeperfx.log` (ve iz sürümünde `psp_trace.txt`) okunur.

## 6. Bilinen sınırlamalar ve açık konular

- Gerçek PSP emülatörden ~3-5 kat yavaş; hafıza kartı erişimleri emülatörde
  görünmeyen ek gecikme yaratıyor. Süreler gerçek cihazda ölçülerek iyileştiriliyor
  (`keeperfx.log`'daki `PSP load:` satırları).
- Bellek payı gerçek cihazda henüz ölçülmedi; son sürümdeki `PSP memory` log satırları bunu verecek.
- Çok oyunculu mod yok; en büyük harita 85x85; harita başına en fazla 254
  yaratık (log'daki `MAPCREATURELIMIT` uyarıları bundan).
- Kayıt dosyaları masaüstü sürümüyle uyumlu değil (farklı yapı düzeni).
- İlk açılış, sprite önbelleğini yazdığı için sonrakilerden yavaştır.

### Sorun 24: Elde tutulan imp zindana bırakılamıyor
- **Belirti:** Panelden alınan imp elde görünüyor, küp yeşil/kırmızı oluyor ama × bir şey yapmıyor.
- **Neden:** DK'de eldeki yaratığı bırakmak sağ tık; × sol tık olduğu için sadece büyü/oda işleri yapıyordu.
- **Çözüm:** `packets_input.c` içinde PSP'ye özel: sol tık bırakıldığında el doluysa ve imleç altında alınacak yaratık yoksa `dump_first_held_thing_on_map` çağrılıyor. Kırmızı küpte bırakma başarısız olur, hiçbir şey olmaz. Yaratığın üstündeyken × onu almaya devam eder.
