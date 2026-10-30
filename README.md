# ESP8266 NodeMCU — Webserver điều khiển 4 relay

**Mới:** nhập Wi-Fi nhà trong Cài đặt, đồng bộ 0/1 qua Firebase và web điều khiển deploy Vercel trong `web/`. Xem [hướng dẫn Wi-Fi / Firebase / Vercel](FIREBASE_SETUP.md).

Firmware Arduino C++ cho NodeMCU ESP8266 (kể cả board dùng chip USB CH340). Chữ “Lua” trong tên sản phẩm không bắt buộc viết Lua: nạp firmware này sẽ thay firmware đang có trên board.

## Thử giao diện ngay khi chưa có ESP8266

Máy chủ mô phỏng dùng **đúng HTML/CSS/JavaScript từ firmware**, kèm API mô phỏng 4 relay, đồng hồ và lịch. Chỉ cần Node.js 18 trở lên; chạy máy chủ không cần cài npm package, Arduino hay ESP8266.

**Trên Windows:** nhấp đúp `start-test.cmd`, giữ cửa sổ đó mở rồi truy cập **http://localhost:8266**. Hoặc chạy trong thư mục dự án:

```sh
node simulator/server.cjs
```

Giao diện dùng bố cục trắng/xanh, 4 thẻ kênh dạng lưới, công tắc trượt và nút Bật/Tắt riêng. Thanh dưới gồm **Trang chủ**, **Lịch**, **Thiết bị**, **Cài đặt**. Cả Trang chủ và tab **Lịch** đều chứa đủ tối đa 10 lịch trong vùng cuộn riêng: kéo thanh cuộn, lăn chuột hoặc vuốt lên/xuống trên điện thoại để xem các lịch phía dưới. Danh sách giữ vị trí cuộn khi tự cập nhật trạng thái. Bấm vào nội dung lịch để sửa, công tắc để tạm dừng/kích hoạt và nút **⋮** để mở thao tác xóa. Đồng hồ và nút đồng bộ nằm trong **Cài đặt**.

Cuối nội dung trang có mục thu gọn **MÔ PHỎNG · Công cụ thử nghiệm**. Bấm mở mục này để dùng:

- **+1 phút / +1 giờ / +1 ngày:** tua đồng hồ để kiểm tra mốc bật/tắt.
- **Đặt ngày giờ để thử lịch:** chọn ngày, giờ cụ thể theo múi giờ trình duyệt. Khi thử giờ, tự đồng bộ từ điện thoại tạm dừng để không ghi đè giờ đã chọn.
- **Lấy giờ thật** hoặc **Đồng bộ giờ:** quay lại đồng hồ thực và bật lại đồng bộ tự động.
- **Mô phỏng mất điện:** relay OFF, mất giờ, lịch vẫn giữ. Máy mô phỏng tạm dừng tự đồng bộ để bạn quan sát trạng thái này; bấm **Lấy giờ thật** để tiếp tục.

Ví dụ test nhanh: thêm lịch **Relay 1 + Relay 2**, **22:00 → 02:00**, **Chủ nhật**; đặt giờ thử **25/10/2026 21:59** rồi bấm **+1 phút**. Hai relay sẽ bật. Bấm **+1 giờ** bốn lần để tới 02:00 Thứ 2 và thấy relay tắt.

Lịch thử lưu ở `.simulator/config.json`, giữ nguyên khi tải lại trang hoặc khởi động lại máy chủ. Mỗi lần khởi động máy chủ, giờ được lấy lại khi mở trang. Đồng hồ mô phỏng chạy ở tốc độ bình thường giữa các lần tua. Tất cả tab/điện thoại dùng chung trạng thái trên máy chủ.

**Thử bằng điện thoại:** cho điện thoại và máy tính vào cùng Wi-Fi nhà, dừng máy chủ cũ nếu đang chạy rồi chạy:

```sh
node simulator/server.cjs --lan
```

Mở địa chỉ **Phone / LAN** in trong terminal, ví dụ `http://192.168.1.10:8266`. Điện thoại không dùng `localhost` hoặc `192.168.4.1` trong chế độ mô phỏng. Nếu Windows Firewall hỏi, cho phép Node.js trên mạng riêng; nếu chưa truy cập được, kiểm tra chặn kết nối giữa các thiết bị trên router. Không cần mở cổng router.

Nhấn **Ctrl+C** để dừng máy chủ. Nếu cổng bị chiếm, mở URL hiện có hoặc đổi cổng trong PowerShell:

```powershell
$env:PORT = '8267'
node simulator/server.cjs
```

Đây là mô phỏng API và hành vi lịch bằng Node.js, không phải giả lập vi điều khiển; không kiểm tra được GPIO, nguồn điện, Wi-Fi AP hay relay vật lý. Thanh công cụ thử chỉ có trên máy tính, không được đưa vào firmware.

Kiểm tra máy chủ và logic mô phỏng (không cần thư viện ngoài):

```sh
node --test tests/simulator_test.cjs
```

Bài kiểm tra giao diện với máy chủ HTTP thật dùng jsdom đã mô tả ở cuối tài liệu; sau khi cài thư viện đó, chạy `node tests/simulator_ui_test.cjs`.

## Chức năng

- **Wi-Fi riêng luôn hoạt động**, có thể nhập Wi-Fi router trong Cài đặt để dùng Firebase. Local không cần Internet.
- Giao diện tiếng Việt trên điện thoại, không dùng CDN hay thư viện tải ngoài.
- Bật/tắt độc lập 4 relay bằng công tắc hoặc nút Bật/Tắt.
- Đổi tên 4 thiết bị tại **Cài đặt → Tên thiết bị**; tên hiển thị trên thẻ điều khiển, danh sách lịch và màn hình chọn kênh.
- Tối đa **10 lịch tổng cộng**. Mỗi lịch chọn một hoặc nhiều relay, giờ bật/tắt, các thứ trong tuần; có thể sửa, xóa hoặc tạm dừng.
- Lưu lịch và múi giờ vào LittleFS, giữ lại sau khi mất nguồn.
- Tự đồng bộ giờ và múi giờ từ điện thoại khi mở trang, đồng bộ lại mỗi 5 phút khi trang đang mở. Có nút đồng bộ thủ công.
- Đồng hồ và lịch tiếp tục chạy khi điện thoại ngắt Wi-Fi, miễn ESP còn nguồn.

## Đổi tên thiết bị

Mở **Cài đặt → Tên thiết bị**, nhập tên cho từng kênh (ví dụ **Đèn phòng khách**, **Quạt trần**, **Ổ cắm**, **Đèn sân**) rồi bấm **Lưu tên thiết bị**. Mỗi tên từ 1–24 ký tự, hỗ trợ tiếng Việt. Các ô vẫn giữ nội dung đang nhập khi giao diện tự cập nhật trạng thái.

Tên được lưu trong flash cùng cấu hình và giữ lại sau mất nguồn. Trên máy mô phỏng, tên được lưu tại `.simulator/config.json`. Đổi tên không thay đổi số kênh, trạng thái bật/tắt hay liên kết với lịch đã tạo. Cấu hình cũ chưa có tên vẫn được đọc bình thường và dùng tên mặc định Kênh 1–4.

**Firmware không chứa công cụ mô phỏng.** HTML/CSS/JavaScript của công cụ này chỉ nằm trong thư mục `simulator/` và được máy chủ Node.js chèn vào trang trên máy tính. Bản firmware không có nút tua giờ, mô phỏng mất điện hoặc API `/sim/*`.

## Đấu chân điều khiển

| Kênh module relay | Chân NodeMCU | GPIO |
| --- | --- | --- |
| IN1 | D1 | 5 |
| IN2 | D2 | 4 |
| IN3 | D5 | 14 |
| IN4 | D6 | 12 |

Chọn module relay có đầu vào điều khiển tương thích **logic 3,3 V**. Cấp nguồn module theo đúng điện áp module; không cấp nguồn cuộn relay từ chân GPIO hoặc chân 3V3. Với module điều khiển thông thường không cách ly, nối chung GND giữa module và NodeMCU. Nếu module có optocoupler/JD-VCC, đấu nguồn theo tài liệu của chính module.

Mặc định **active LOW**: GPIO LOW thì relay bật. Với module kích mức HIGH, sửa `RELAY_ACTIVE_LOW = false` trong `firmware/esp8266_4relay/config.h`. Khi chương trình bắt đầu, firmware đặt cả 4 relay về OFF. Trong giai đoạn reset trước khi chương trình chạy, GPIO có thể thả nổi; dùng điện trở kéo về mức OFF hoặc mạch enable phù hợp nếu cần bảo đảm relay không nháy lúc cấp nguồn. Thử với tải điện áp thấp trước khi nối tải điện lưới.

Ánh xạ chân theo [định nghĩa NodeMCU trong ESP8266 Arduino Core](https://github.com/esp8266/Arduino/blob/master/variants/nodemcu/pins_arduino.h).

## Nạp bằng Arduino IDE

1. Trong **Preferences → Additional Boards Manager URLs**, thêm:
   `https://arduino.esp8266.com/stable/package_esp8266com_index.json`
2. Cài board package **esp8266 by ESP8266 Community**, phiên bản **3.1.2**.
3. Trong Library Manager, cài **ArduinoJson by Benoit Blanchon**, phiên bản **6.21.5**.
4. Mở `firmware/esp8266_4relay/esp8266_4relay.ino`. Các file `.h` phải ở cùng thư mục với `.ino`.
5. Chọn board **NodeMCU 1.0 (ESP-12E Module)**, Flash Size **4MB (FS:1MB OTA:~1019KB)** và đúng cổng COM của CH340.
6. Sửa tên Wi-Fi, mật khẩu và mức kích relay trong `config.h` nếu cần, rồi **Upload**. Nếu máy không thấy cổng COM, kiểm tra dây USB truyền dữ liệu và driver CH340.

Không cần upload filesystem riêng: trang web đã nhúng trong firmware, LittleFS chỉ chứa cấu hình. Serial Monitor dùng **115200 baud**. Không chọn xóa toàn bộ flash nếu muốn giữ lịch đã lưu.

## Nạp bằng PlatformIO

Mở thư mục dự án bằng VS Code + PlatformIO, sau đó:

```sh
pio run
pio run --target upload
pio device monitor
```

`platformio.ini` đã chọn `nodemcuv2`, ESP8266 platform 4.2.1 và ArduinoJson 6.21.5. Nếu có nhiều cổng COM, đặt `upload_port = COMx` trong file này. Cấu hình board theo [tài liệu PlatformIO ESP8266](https://docs.platformio.org/en/latest/platforms/espressif8266.html).

## Dùng trên điện thoại

1. Cấp nguồn ESP8266.
2. Kết nối Wi-Fi **ESP8266-4Relay**, mật khẩu mặc định **12345678** (theo `config.h`).
3. Nếu điện thoại báo “Không có Internet”, chọn **vẫn kết nối**. Nếu trình duyệt không vào được, tắt tính năng tự chuyển sang dữ liệu di động/VPN.
4. Mở trình duyệt và nhập **http://192.168.4.1** (HTTP, không phải HTTPS).
5. Trang tự gửi giờ điện thoại sang ESP; kiểm tra đồng hồ ESP hiển thị đúng trước khi tạo lịch.
6. Bấm **Thêm lịch**, chọn relay, giờ bật, giờ tắt và các thứ, rồi **Lưu lịch**.

Ví dụ: chọn **Relay 1 + Relay 2**, **08:00 → 10:30**, **Thứ 2 + Thứ 4 + Thứ 6**. Hai relay bật trong khoảng đó và tắt khi hết lịch, trừ khi một lịch khác vẫn yêu cầu bật.

Wi-Fi và địa chỉ IP dùng [SoftAP của ESP8266](https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/soft-access-point-class.html). Bất kỳ người nào biết mật khẩu và kết nối được Wi-Fi này đều có quyền điều khiển, sửa lịch và đồng bộ giờ; hãy đổi mật khẩu mặc định trước khi sử dụng.

## Quy tắc lịch và bật/tắt tay

- Khoảng giờ gồm thời điểm bắt đầu, không gồm thời điểm kết thúc: `08:00 → 09:00` tắt từ 09:00.
- Nếu giờ tắt nhỏ hơn giờ bật, lịch **qua nửa đêm**. Ví dụ Thứ 2 `22:00 → 02:00` chạy đến 02:00 Thứ 3. Thứ được chọn luôn là ngày bắt đầu, kể cả Chủ nhật sang Thứ 2.
- Không chấp nhận giờ bật bằng giờ tắt để tránh nhầm thời lượng 0 giờ/24 giờ.
- Nếu nhiều lịch trùng nhau, relay bật khi **ít nhất một lịch** còn hoạt động. Hết một lịch không làm tắt relay nếu lịch khác vẫn cần bật.
- Bật/tắt tay ghi đè trạng thái hiện tại đến lần kế tiếp mà **trạng thái bật/tắt tổng hợp của lịch trên relay đó** thay đổi. Mốc bắt đầu/kết thúc của lịch trùng nhau không xóa ghi đè nếu trạng thái tổng hợp vẫn như cũ. Không có lịch làm đổi trạng thái thì lệnh tay giữ nguyên đến khi bạn sửa lịch hoặc khởi động lại.
- API `/api/auto` vẫn cho phép áp dụng lịch ngay cho một relay; nếu không có lịch hoạt động, relay tắt. Giao diện không hiển thị nút này.
- Thêm/sửa/xóa/kích hoạt/tạm dừng lịch đưa **tất cả relay** về theo lịch ngay lập tức.
- Lần đồng bộ giờ đầu tiên sau khởi động cũng đưa tất cả relay về theo lịch. Đồng bộ giờ các lần sau giữ lệnh tay, trừ khi giờ mới làm trạng thái lịch thay đổi.
- Lưu lịch thành công mới áp dụng vào RAM; nếu không lưu được, API báo lỗi và giữ nguyên lịch đang dùng. Lưu qua file tạm rồi rename để tránh ghi đè trực tiếp file cấu hình.

## Giờ sau khi mất điện

ESP8266 không có RTC kèm pin. Khi có Wi-Fi và Internet, firmware lấy giờ NTP; khi chỉ dùng local, sau reset hoặc mất nguồn:

- Lịch vẫn còn trong flash, nhưng giờ hiện tại chưa biết.
- Relay khởi động ở OFF; lịch tạm dừng đến khi điện thoại mở trang để đồng bộ. Bật/tắt tay vẫn hoạt động trước khi có giờ.
- Khi nhận giờ, ESP tính trạng thái cần có **ngay lúc đó**, không phát lại các mốc lịch đã qua trong thời gian tắt nguồn.

Trình duyệt chỉ gửi được giờ **khi mở trang**, không phải ngay lúc điện thoại vừa kết nối Wi-Fi. Giờ phụ thuộc vào độ đúng của đồng hồ điện thoại; bộ đếm ESP có thể trôi khi chạy lâu. Múi giờ lấy từ điện thoại, mặc định UTC+7; nếu dùng nơi có DST, cần mở lại trang khi đổi giờ mùa hè. Nếu muốn lịch tự chạy lại sau mất điện mà không cần điện thoại, cần bổ sung RTC có pin, ví dụ DS3231; chức năng đó chưa có trong firmware này.

## API cục bộ

GET trả trạng thái; các POST thành công trả cùng cấu trúc trạng thái mới. POST bắt buộc có `Content-Type: application/json`.

| Method | Đường dẫn | Body ví dụ |
| --- | --- | --- |
| GET | `/api/state` | — |
| POST | `/api/relay` | `{"relay":1,"on":true}` |
| POST | `/api/auto` | `{"relay":1}` |
| POST | `/api/time` | `{"epoch":1792717200,"utcOffsetMinutes":420}` |
| POST | `/api/names` | `{"names":["Đèn phòng khách","Quạt trần","Ổ cắm","Đèn sân"]}` |
| POST | `/api/wifi` | `{"ssid":"Wi-Fi nhà","password":"matkhauwifi"}` |
| POST | `/api/schedule` | `{"id":-1,"enabled":true,"relays":3,"days":21,"start":480,"end":630}` |
| POST | `/api/schedule/delete` | `{"id":0}` |

`epoch` là Unix time UTC theo **giây**; dùng giờ thực tế khi gọi API. `id=-1` tạo lịch; `id=0..9` sửa/xóa lịch đã có. `relays` là bitmask: relay 1=1, 2=2, 3=4, 4=8. `days` là bitmask: Thứ 2=1, Thứ 3=2, …, Chủ nhật=64. Ví dụ `21=1+4+16` là Thứ 2/4/6. `start`, `end` là phút tính từ 00:00. Lịch tạm dừng vẫn chiếm một trong 10 vị trí.

`/api/names` nhận đủ 4 tên theo thứ tự kênh 1–4; chỉ áp dụng khi toàn bộ tên hợp lệ và lưu thành công. Trạng thái trả về bổ sung trường `name` cho mỗi phần tử trong `relays`.

## Kiểm tra logic trên máy tính

```sh
g++ -std=c++11 -Wall -Wextra -pedantic tests/scheduler_test.cpp -o scheduler_test.exe
./scheduler_test.exe
```

Bài kiểm tra bao phủ mốc bật/tắt, lịch trùng, lịch qua ngày/qua tuần, ghi đè tay, múi giờ, mốc năm 2038 và tràn bộ đếm `millis()`. Việc thử GPIO và module relay thực tế cần board được nối USB/nguồn tại chỗ.

Kiểm tra thao tác giao diện bằng DOM mô phỏng và API giả lập:

```sh
npm install --prefix .tools/ui-test --cache .tools/npm-cache jsdom --no-audit --no-fund
node tests/ui_test.cjs
```

Đã biên dịch thành công bằng ESP8266 Arduino Core 3.1.2, board `nodemcuv2`, flash 4 MB / filesystem 1 MB. Cả bài kiểm tra scheduler và giao diện đều đạt. Bản binary đã tạo tại `build/firmware/esp8266_4relay.ino.bin`; file này dùng cấu hình Wi-Fi và active LOW mặc định. Chưa nạp hoặc kiểm tra trên board/relay thực tế.
