# Wi-Fi, Firebase và Vercel

## 1. ESP8266 / giao diện local

Nạp firmware mới từ `build/firmware/esp8266_4relay.ino.bin` hoặc compile sketch bằng Arduino/PlatformIO như trước. Kết nối Wi-Fi `ESP8266-4Relay`, mở `http://192.168.4.1`, vào **Cài đặt → Kết nối Wi-Fi nhà**, nhập SSID và mật khẩu mạng **2.4 GHz**, chọn **Lưu & kết nối**. Mạng mở có thể để trống mật khẩu.

SSID/mật khẩu được lưu riêng trong LittleFS (`/wifi.json`); cấu trúc lịch và tên thiết bị không đổi. Mật khẩu không được trả về API hoặc đưa lên Firebase. Wi-Fi AP vẫn mở khi ESP kết nối router; radio có thể đổi kênh nên điện thoại có thể phải kết nối lại AP. Đổi mật khẩu AP mặc định trong `config.h` trước khi sử dụng thực tế.

Khởi động ở chế độ AP-only, chờ 30 giây trước khi thử Wi-Fi nhà đã lưu. Kết nối thử tối đa 20 giây; nếu thất bại, tắt riêng STA và giữ AP. Tự thử lại sau ít nhất 30 giây tính từ đầu lượt trước, nhưng hoãn khi có điện thoại đang nối AP local; sau khi điện thoại rời AP, chờ thêm 30 giây. Bấm **Lưu & kết nối** luôn thử ngay. Không có Wi-Fi: không gọi Firebase, điều khiển local và lịch vẫn chạy nếu đã có giờ. Có Internet: lấy giờ NTP để xác minh HTTPS và khởi tạo đồng hồ lịch khi cần. Nếu NTP bị chặn, Firebase chờ giờ hợp lệ; tab Cài đặt hiển thị trạng thái này.

Nếu thấy SSID nhưng không nối được: chọn **Quên mạng**, kết nối lại `ESP8266-4Relay` bằng mật khẩu **12345678** theo `config.h` (không dùng mật khẩu `relay1234` trong tài liệu cũ). Nếu điện thoại báo không có Internet, chọn giữ kết nối. Mở Serial Monitor 115200 baud: kiểm tra `configuration: OK, started: OK`, địa chỉ AP, thông báo `Local Wi-Fi: client joined/disconnected` và lý do reset. Sau khi sửa firmware phải nạp lại board; chỉ sửa `config.h` trên máy không thay đổi mật khẩu của firmware đang chạy.

Nếu watchdog reset sau **Lưu & kết nối**, nạp binary mới: thao tác nối router được xếp hàng để chạy sau khi HTTP handler kết thúc (chờ 500 ms); các bộ đệm JSON của lưu Wi-Fi và đồng bộ Firebase dùng heap để giảm stack. Serial in các mốc `[net] Saving Wi-Fi`, `Wi-Fi saved`, `Wi-Fi begin`, `Wi-Fi begin returned`, `Router connected; starting NTP`, `Firebase HTTPS request` và mã HTTP. Mỗi mốc ghi lượng heap/stack còn trống, không ghi mật khẩu. Nếu vẫn reset, lưu log từ lúc bấm nút đến hết phần crash, đặc biệt mốc `[net]` cuối cùng. Build/test trên máy chưa xác nhận hết lỗi trên phần cứng; không tắt watchdog để che lỗi.

## 2. Cấu trúc dữ liệu chung

Database: `https://iot-thx-default-rtdb.asia-southeast1.firebasedatabase.app`

Đường dẫn thiết bị: `/devices/esp8266_4relay`

```json
{
  "relays": { "1": 0, "2": 0, "3": 0, "4": 0 },
  "reported": { "1": 0, "2": 0, "3": 0, "4": 0 },
  "names": { "1": "Kênh 1", "2": "Kênh 2", "3": "Kênh 3", "4": "Kênh 4" },
  "lastSeen": 1793318400000
}
```

- `relays/{1..4}`: giá trị điều khiển chính, **số** `0` = tắt, `1` = bật. Web PUT đúng một kênh; ESP PATCH các kênh vừa thay đổi do local/lịch, không ghi đè toàn bộ thiết bị.
- `reported/{1..4}`: đầu ra GPIO mà firmware đã áp dụng; không phải cảm biến xác minh tiếp điểm relay.
- `names`: tên được đặt từ local. `lastSeen`: timestamp máy chủ Firebase, cập nhật khi ESP đồng bộ thành công.
- ESP và web polling khoảng 2 giây, cộng thời gian HTTPS. ESP cập nhật `reported`/`names`/`lastSeen` khi có thay đổi hoặc mỗi 10 giây nếu không đổi. Chứng chỉ được parse một lần và TLS session được tái sử dụng khi máy chủ hỗ trợ. Đây là đồng bộ gần thời gian thực, không phải stream. Web đánh dấu ESP chưa phản hồi sau 20 giây.
- Firebase thiếu kênh: ESP khởi tạo từng kênh bằng trạng thái hiện tại, với conditional PUT để tránh đè lệnh vừa đến từ web.
- Sau mất mạng/khởi động lại, giá trị Firebase có sẵn được ưu tiên. Thay đổi local khi offline không được xếp hàng để ghi đè Firebase lúc kết nối lại. Khi online, thay đổi local/lịch được gửi trước lượt đọc tiếp theo. Nếu nhiều nguồn ghi cùng một kênh, lần ghi cuối tại Firebase thắng.
- Lệnh cloud có cùng cách xử lý manual như lệnh local. Lịch vẫn có thể thay đổi trạng thái ở mốc tiếp theo. Đọc lại giá trị không đổi không reset manual.

## 3. Quyền Firebase

Chỉ URL và đường dẫn chưa cấp quyền đọc/ghi. Mã hiện dùng Firebase REST; mặc định không gửi token. Nếu Rules đang khóa, UI hiển thị lỗi quyền truy cập, local vẫn dùng được. Repo không tự thay đổi Rules của dự án.

Kiểm tra GET không token ngày 30/10/2026 tại đường dẫn `relays.json` trả **HTTP 401**. Cần cấp quyền thích hợp trước khi ESP/web đồng bộ thật.

Để **thử nghiệm trên thiết bị không nối tải quan trọng**, có thể dùng `firebase.demo.rules.json` làm mẫu Rules. Mẫu chỉ mở đường dẫn thiết bị này, giới hạn giá trị kênh về số 0/1. **Mọi người biết URL đều có thể điều khiển các kênh khi dùng mẫu này.** Chỉ áp dụng trong môi trường thử nghiệm và khóa lại sau khi thử. Khi ghép vào Rules hiện có, giữ nguyên quyền của các đường dẫn khác; quyền rộng ở cấp cha vẫn có hiệu lực.

Nếu database yêu cầu Firebase Auth: firmware hỗ trợ `FIREBASE_AUTH_TOKEN` trong `config.h`; web có ô ID token trong **Cấu hình quyền truy cập**. ID token có hạn dùng và phiên bản này **chưa có đăng nhập/refresh token tự động**. Không đưa database secret/service-account key vào frontend hoặc commit token thật. Muốn vận hành lâu dài với Rules riêng tư cần bổ sung cơ chế Firebase Auth phù hợp tài khoản của bạn.

HTTPS firmware xác minh chứng chỉ với GTS Root R1–R4 trong `firebase_ca.h`, lấy từ Google Trust Services; không bỏ xác minh TLS. Thay đổi chuỗi CA phía dịch vụ có thể cần cập nhật các root. HTTP/TLS là đồng bộ và có timeout, nên các lần kết nối Internet chậm có thể làm local/lịch phản hồi trễ vài giây; cần kiểm chứng trên board thật.

## 4. Deploy Vercel

1. Đưa repo lên GitHub rồi import project trên Vercel.
2. **Root Directory**: thư mục gốc repo. **Framework Preset**: Other.
3. `vercel.json` đã đặt **Output Directory = web**, không cần install/build. Chỉ thư mục `web/` được phát hành.
4. Deploy, mở URL Vercel. Web dùng cùng Firebase đã cấu hình sẵn trong `web/app.js`.

Hoặc dùng Vercel CLI tại thư mục gốc: `vercel`, rồi `vercel --prod` khi muốn phát hành production. Không cần Firebase SDK/API key nếu Rules cho phép REST không đăng nhập. Wi-Fi phải cấu hình từ local, không nhập mật khẩu Wi-Fi trên web Vercel.

## Kiểm tra

Log exception 29 (`epc1=0x4000df64`, `excvaddr=0`) đã được đối chiếu với ELF của bản `v0007eb70`: nhánh hệ thống chạy `ieee80211_setup_ratetable`/`scan_parse_beacon` trong khi nhánh ứng dụng đang bắt tay TLS cho PATCH báo cáo sau GET. GET đã nhận và áp dụng lệnh web thành công. Đây không còn là lỗi đọc JSON; stack chỉ vào SDK Wi-Fi. Áp lực heap là nghi vấn từ log, chưa được chứng minh hoàn toàn trên phần cứng. Có [báo cáo cùng dấu hiệu trong SDK Espressif](https://github.com/espressif/ESP8266_NONOS_SDK/issues/320).

Bản sửa tách GET, khởi tạo kênh và PATCH báo cáo sang các lượt loop khác nhau, mỗi lượt tối đa một HTTP request và cách nhau ít nhất 2 giây. Bộ đệm JSON được giải phóng trước handshake. Trước tạo TLS client, firmware yêu cầu heap trống ít nhất 32.000 byte và khối trống lớn nhất ít nhất 18.000 byte; nếu thiếu sẽ báo `TLS deferred: low heap` và giữ local hoạt động. Đây là ngưỡng dự phòng, không phải bảo đảm mọi lỗi SDK đã được loại bỏ. Log bổ sung `maxblock`; lỗi PATCH báo cáo không chặn việc đọc lệnh ở lượt sau. Khi có báo cáo/khởi tạo xen kẽ, chu kỳ đọc lệnh dài hơn 2 giây; UI xác nhận ESP cũng có thể chậm hơn một lượt.

Chạy `node tests/run_firebase_sync_test.cjs` để test trực tiếp các hàm đồng bộ của firmware với HTTP giả lập: một request mỗi lượt, local ON → web OFF, khởi tạo kênh và lỗi báo cáo không chặn đọc lệnh.

Sửa lỗi restart ở bản tối ưu TLS: không dùng `static X509List(FIREBASE_CA)` trong hàm. ESP8266 Core 3.1.2 khóa ngắt bằng `__cxa_guard_acquire` suốt constructor của static cục bộ; parse chứng chỉ ở đó có thể gây watchdog. Firmware chuẩn bị chứng chỉ một lần bằng `prepareFirebaseTls()` với ngắt hoạt động, rồi trả về loop trước khi mở HTTPS. Serial phải có `[net] TLS roots parse begin` và `[net] TLS roots ready`. Test `node tests/tls_init_test.cjs` kiểm tra binary đã compile để ngăn đưa lại static initializer này.

Đã sửa lỗi chiều web → ESP: `FirebaseBody` phải khai báo `availableForWrite()` cho HTTPClient của ESP8266. Nếu thiếu, HTTP có thể trả 200 nhưng bộ nhận chờ timeout, JSON rỗng và lệnh không được áp dụng. Bản mới nhận theo block, giới hạn 4096 byte, trả lỗi khi đọc body thất bại thay vì coi HTTP 200 là thành công. Serial hiển thị method, đường dẫn, số byte và `Applied web relay N = 0/1` khi áp dụng một giá trị khác đầu ra hiện tại.

Test C++ dùng chính bộ nhận, parser Firebase và logic relay của firmware, kiểm tra object/array, local ON → web OFF, dữ liệu sai, giới hạn bộ nhớ và hết heap. Chạy với GCC cùng thư viện ArduinoJson 6 đã cài:

```sh
g++ -std=c++11 -Wall -Wextra -I .tools/arduino-user/libraries/ArduinoJson/src tests/firebase_transport_test.cpp -o build/firebase_transport_test.exe
./build/firebase_transport_test.exe
```

```sh
node tests/ui_test.cjs
node tests/simulator_test.cjs
node tests/simulator_ui_test.cjs
node tests/cloud_ui_test.cjs
```

Simulator vẫn chỉ mô phỏng relay/lịch; form Wi-Fi bị vô hiệu hóa trong simulator. Trên board thật: thử đúng/sai mật khẩu, mất router, mất Internet, bật/tắt trên local và web, thay đổi lịch và khởi động lại. Xác minh `relays` đổi 0/1 và `reported` cập nhật theo. Chưa có kiểm chứng phần cứng hoặc deployment Vercel chỉ bằng việc chạy các test mô phỏng.

Tài liệu: [Firebase REST](https://firebase.google.com/docs/database/rest/save-data), [conditional requests](https://firebase.google.com/docs/reference/rest/database), [Vercel configuration](https://vercel.com/docs/project-configuration/vercel-json).
