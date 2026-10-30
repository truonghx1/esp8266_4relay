#pragma once
#include <Arduino.h>

const char WEB_UI[] PROGMEM = R"HTML(<!doctype html>
<html lang="vi">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#f8fbff">
<title>Điều khiển Relay 4 Kênh</title>
<style>
:root{color-scheme:light;--ink:#172343;--muted:#73839e;--line:#edf2f8;--accent:#2184ff;--blue:#0964dc;--paper:#f8fbff;--green:#11b76a;--red:#f44c5d}
*{box-sizing:border-box}body{margin:0;background:#edf3fa;color:var(--ink);font:14px/1.5 -apple-system,BlinkMacSystemFont,"Segoe UI",Arial,sans-serif;-webkit-tap-highlight-color:transparent}main{position:relative;max-width:480px;min-height:100dvh;margin:auto;padding:22px 16px 108px;background:linear-gradient(160deg,#fff 5%,#f8fbff 70%)}h1,h2,h3,p{margin:0}h1{font-size:23px;line-height:1.25;letter-spacing:-.7px;font-weight:800}h2{font-size:19px;letter-spacing:-.4px}h3{font-size:14px}button,input{font:inherit}button{cursor:pointer;border:0;color:inherit;background:transparent;touch-action:manipulation}button:disabled{cursor:default;opacity:.48}button:focus-visible,input:focus-visible,summary:focus-visible{outline:3px solid #91c7ff;outline-offset:3px}.icon{display:block;width:22px;height:22px;flex-shrink:0;fill:none;stroke:currentColor;stroke-width:1.8;stroke-linecap:round;stroke-linejoin:round}.icon.small{width:15px;height:15px}.svg-defs{position:absolute;width:0;height:0;overflow:hidden}.muted{color:var(--muted);font-size:12px}.note{font-size:12px;color:var(--muted);line-height:1.65;margin-top:12px}[hidden]{display:none!important}
.app-header{display:flex;justify-content:space-between;align-items:center;gap:6px;margin-bottom:18px}.icon-button{display:grid;place-items:center;min-width:40px;height:40px;border-radius:50%;color:#5d718f;background:#eef5fd}.device-strip{width:100%;display:flex;align-items:center;gap:11px;padding:12px;background:linear-gradient(110deg,#f0f7ff,#e9f3ff);border-radius:12px;text-align:left;margin-bottom:16px}.chip-icon{display:grid;place-items:center;width:38px;height:42px;border-radius:9px;background:#deedff;color:#0065d7;flex-shrink:0}.chip-icon .icon{width:27px;height:27px}.device-copy{min-width:0;flex:1}.device-name{font-size:12px;font-weight:500;white-space:nowrap}.device-name strong{font-weight:700}.connection-line{display:flex;align-items:center;gap:5px;margin-top:3px;font-size:10px;color:#637795;white-space:nowrap}.connection-line::before{content:"";width:6px;height:6px;background:#c0cad8;border-radius:50%}.connection-line.live::before{background:var(--green)}#connection{color:#e34b5b}.live #connection{color:#00a5a8}.ip-pill{display:flex;align-items:center;gap:5px;font-size:10px;color:#0875ec;background:#deefff;border-radius:20px;padding:5px 8px;white-space:nowrap;max-width:135px;overflow:hidden}.ip-pill span{overflow:hidden;text-overflow:ellipsis}.chevron{color:#8ca0b9;width:16px;height:16px}
.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px}.relay{min-width:0;padding:13px 11px 10px;background:linear-gradient(145deg,#fff,#fbfdff);border:1px solid #eaf0f8;border-radius:13px;box-shadow:0 3px 7px #163f7010}.relaytop{display:flex;align-items:center;gap:8px;min-width:0}.channel-icon{height:36px;width:36px;flex-shrink:0;display:grid;place-items:center;border-radius:50%;background:#edf0f5;color:#65758d}.channel-icon .icon{width:23px;height:23px}.channel-icon.kind-1{background:#fff4cf;color:#e8b300}.channel-icon.kind-2{background:#dfedff;color:#2978c7}.channel-icon.kind-3{background:#edf0f4;color:#63728b}.channel-icon.kind-4{background:#e9edf3;color:#5f6d83}.relay-copy{min-width:0;flex:1}.relay h3{line-height:1.25;font-weight:750;white-space:nowrap}.relay-status{font-size:10px;color:var(--red);display:flex;align-items:center;gap:4px;margin-top:3px;white-space:nowrap}.relay-status::before{content:"";width:5px;height:5px;flex-shrink:0;background:currentColor;border-radius:50%}.relay.on .relay-status{color:var(--green)}.switch{position:relative;width:38px;min-width:38px;height:44px;padding:0;border-radius:22px;background:transparent}.switch::before{content:"";position:absolute;left:0;right:0;top:11px;bottom:11px;border-radius:20px;background:#c9d2de;transition:background .15s}.switch::after{content:"";position:absolute;top:13px;left:2px;width:18px;height:18px;border-radius:50%;background:#fff;box-shadow:0 1px 4px #17324a30;transition:transform .15s}.switch[aria-checked=true]::before{background:var(--accent)}.switch[aria-checked=true]::after{transform:translateX(16px)}.relay-buttons{display:flex;gap:7px;margin-top:10px}.power-button{flex:1;display:flex;align-items:center;justify-content:center;gap:5px;min-height:36px;padding:7px 3px;border-radius:7px;background:#edf5ff;color:#5f7d9f;font-size:12px}.power-button .icon{width:15px;height:15px}.power-button.selected-on{background:linear-gradient(135deg,#3194ff,#1878f5);color:white;box-shadow:0 3px 7px #1880ff22}.power-button.selected-off{background:#fff0f2;color:#f23b50}
.sectionhead{display:flex;align-items:center;justify-content:space-between;gap:8px;margin:24px 0 12px}.section-title{display:flex;align-items:center;gap:9px}.section-title>.icon{color:#0863c9;width:25px;height:25px}.see-all{display:flex;align-items:center;gap:2px;font-size:11px;color:var(--blue);background:#f0f7ff;border:1px solid #deecff;border-radius:20px;padding:5px 9px;min-height:32px}.see-all .icon{width:13px;height:13px}.count{font-size:11px;color:#8393ac;font-weight:500;margin-left:5px}.schedule{position:relative;border:1px solid var(--line);border-radius:10px;background:#fff;margin-top:7px;padding:6px 8px;box-shadow:0 1px 3px #304a6810}.schedule-row{display:flex;align-items:center;gap:9px}.schedule .channel-icon{width:33px;height:33px}.schedule .channel-icon .icon{width:21px;height:21px}.schedule-edit{flex:1;min-width:0;text-align:left;padding:4px 0;min-height:46px}.schedule h3{font-size:12px;font-weight:700;line-height:1.4}.schedule-days{display:flex;align-items:center;gap:5px;margin-top:3px;color:var(--muted);font-size:10px}.schedule-days .icon{width:12px;height:12px}.schedule .switch{width:36px;min-width:36px}.schedule .switch[aria-checked=true]::after{transform:translateX(14px)}.schedule-more{min-width:27px;height:44px;padding:0;display:grid;place-items:center;color:#607da3}.schedule-more .icon{width:18px;height:18px}.schedule .actions{margin:4px 0 3px;border-top:1px solid var(--line);padding-top:8px}.actions{display:flex;gap:8px;flex-wrap:wrap;margin-top:10px}.actions button{padding:8px 12px;border-radius:8px;background:#edf5ff;color:var(--blue);min-height:36px;font-size:12px}.actions .danger{color:#d52e47;background:#fff0f2}.add-schedule{width:100%;display:flex;align-items:center;gap:13px;padding:13px 12px;margin-top:10px;border-radius:11px;background:linear-gradient(110deg,#eaf4ff,#e6f2ff);text-align:left}.add-symbol{display:grid;place-items:center;width:38px;height:38px;flex-shrink:0;background:linear-gradient(140deg,#359bff,#1476f5);color:#fff;border-radius:50%;box-shadow:0 3px 8px #1880ff25}.add-symbol .icon{width:24px;height:24px}.add-copy{flex:1}.add-copy strong{display:block;font-size:13px}.add-copy small{display:block;color:#6985a5;font-size:10px;margin-top:1px}.empty{text-align:center;padding:25px 18px;border:1px dashed #cfe0f3;border-radius:12px;background:#f8fbff;color:#7d90ac;font-size:12px}.empty .icon{color:#a0c5ef;margin:0 auto 8px;width:28px;height:28px}.limit-note{font-size:11px;color:#73839e;text-align:center;margin-top:7px}
.bottom-nav{position:fixed;z-index:10;bottom:0;left:50%;transform:translateX(-50%);width:100%;max-width:480px;display:grid;grid-template-columns:repeat(4,1fr);background:#ffffffed;backdrop-filter:blur(14px);border-top:1px solid #e8eff7;padding:7px 8px max(10px,env(safe-area-inset-bottom));box-shadow:0 -4px 18px #224d7910}.nav-item{position:relative;display:flex;flex-direction:column;align-items:center;gap:4px;min-height:52px;padding:5px 2px 9px;color:#7d8da7;font-size:10px}.nav-item .icon{width:22px;height:22px}.nav-item.active{color:#0879ff;font-weight:700}.nav-item.active::after{content:"";position:absolute;bottom:0;width:22px;height:3px;border-radius:5px;background:var(--accent)}.nav-item.active .home-icon{fill:currentColor;stroke:currentColor}.page-heading{margin:24px 0 14px}.info-card{border:1px solid #e6eef8;border-radius:14px;padding:18px;background:#fff;margin:12px 0}.info-card h3{margin-bottom:10px}.info-row{display:flex;align-items:center;justify-content:space-between;gap:15px;padding:11px 0;font-size:12px;border-bottom:1px solid #f0f4f9}.info-row:last-child{border:0}.info-row span{color:var(--muted)}.info-row strong{font-size:12px;text-align:right;overflow-wrap:anywhere}.clock{font-size:32px;line-height:1.4;font-weight:700;color:#146edb;letter-spacing:1px;font-variant-numeric:tabular-nums}#date{margin:4px 0 14px}.primary{min-height:42px;padding:10px 16px;border-radius:9px;background:var(--accent);color:#fff;font-weight:650}.secondary{min-height:42px;padding:10px 16px;border-radius:9px;background:#edf3fa;color:#586e8e}.notice{padding:11px 13px;border:1px solid #f5dfaa;border-radius:10px;background:#fff8e9;margin:10px 0;font-size:12px}.notice.error{background:#fff0f2;border-color:#f6c6ce;color:#ab3246}.notice.success{background:#eaf8f0;border-color:#c8e9d8;color:#238553}#message{position:fixed;z-index:30;bottom:85px;left:50%;transform:translateX(-50%);width:calc(100% - 32px);max-width:448px;box-shadow:0 5px 25px #26476720}dialog{border:0;border-radius:20px;padding:24px;width:calc(100% - 28px);max-width:440px;max-height:90dvh;overflow:auto;color:var(--ink);background:#fff;box-shadow:0 20px 70px #12294740}dialog::backdrop{background:#12233d70;backdrop-filter:blur(3px)}.dialog-heading{display:flex;justify-content:space-between;align-items:center;margin-bottom:6px}.dialog-heading .icon-button{background:#f1f5fa;width:34px;min-width:34px;height:34px}.dialog-heading h2{font-size:21px}fieldset{border:0;padding:0;margin:20px 0}legend{font-size:13px;font-weight:650;margin-bottom:10px}.choices{display:flex;gap:7px;flex-wrap:wrap}.choice{display:flex;align-items:center;gap:6px;background:#f8fbff;border:1px solid #e5edf7;border-radius:9px;padding:10px;min-height:42px;cursor:pointer;font-size:12px}.choice:has(input:checked){background:#edf6ff;border-color:#a7d1ff;color:#0b68cf}input[type=checkbox]{width:16px;height:16px;accent-color:var(--accent);margin:0}.timeinputs{display:grid;grid-template-columns:1fr 1fr;gap:14px}.timeinputs label{display:grid;gap:8px;font-size:12px;font-weight:650}input[type=time],input[type=datetime-local]{width:100%;min-width:0;background:#f8fbff;border:1px solid #dfeaf6;padding:12px;border-radius:9px;color:var(--ink)}.dialogactions{display:flex;justify-content:flex-end;gap:10px;margin-top:22px}
@media(min-width:600px){main{margin:28px auto 0;min-height:calc(100dvh - 28px);padding:26px 22px 110px;border-radius:24px 24px 0 0;box-shadow:0 8px 60px #17395f12}h1{font-size:25px}.relay{padding:15px 13px 10px}.relay h3{font-size:15px}.relay-status{font-size:11px}.power-button{min-height:40px}.schedule h3{font-size:13px}.schedule-days{font-size:11px}}
@media(max-width:370px){main{padding:18px 12px 100px}h1{font-size:21px}.grid{gap:9px}.relay{padding:10px 8px 7px}.relaytop{gap:5px}.channel-icon{width:29px;height:29px}.relay h3{font-size:12px}.relay-status{font-size:9px}.relay .switch{width:34px;min-width:34px}.relay .switch[aria-checked=true]::after{transform:translateX(12px)}.device-strip{gap:8px;padding:10px}.device-name{font-size:11px}.ip-pill{font-size:9px;padding:5px}.connection-line{font-size:9px}.schedule-row{gap:6px}.schedule h3{font-size:11px}.schedule-more{min-width:22px}}
@media(prefers-reduced-motion:reduce){*{transition:none!important;scroll-behavior:auto!important}}
.name-fields{display:grid;gap:14px;margin:16px 0}.name-field{display:grid;grid-template-columns:64px minmax(0,1fr);align-items:center;gap:10px;font-size:12px;color:var(--muted)}.name-field input{width:100%;min-width:0;border:1px solid #dfeaf6;border-radius:9px;padding:11px 12px;color:var(--ink);background:#f8fbff}.name-field input:focus{background:#fff}.name-hint{margin-top:6px}.relay h3{overflow:hidden;text-overflow:ellipsis}.schedule h3{overflow-wrap:anywhere}.choice{overflow-wrap:anywhere;max-width:100%}
#schedules{max-height:360px;max-height:min(50dvh,360px);overflow-y:auto;overflow-x:hidden;scrollbar-gutter:stable;scrollbar-width:thin;scrollbar-color:#9dbfe8 #edf4fc;overscroll-behavior-y:contain;touch-action:pan-y pinch-zoom;padding:0 5px 3px 0;border-radius:10px;-webkit-overflow-scrolling:touch}
#schedules::-webkit-scrollbar{width:6px}#schedules::-webkit-scrollbar-track{background:#edf4fc;border-radius:8px}#schedules::-webkit-scrollbar-thumb{background:#9dbfe8;border-radius:8px}#schedules:focus-visible{outline:2px solid #91c7ff;outline-offset:3px}.scroll-hint{margin:7px 0 0;text-align:center;font-size:10px;color:var(--muted)}
</style>
</head>
<body><main>
<svg class="svg-defs" xmlns="http://www.w3.org/2000/svg" aria-hidden="true"><defs><symbol id="i-chip" viewBox="0 0 24 24"><rect x="5" y="5" width="14" height="14" rx="2"/><path d="M9 1v4m6-4v4M9 19v4m6-4v4M1 9h4m-4 6h4m14-6h4m-4 6h4"/><rect x="8" y="8" width="8" height="8" rx="1"/></symbol>
<symbol id="i-wifi" viewBox="0 0 24 24"><path d="M3 9a15 15 0 0 1 18 0M6 13a10 10 0 0 1 12 0m-9 4a5 5 0 0 1 6 0"/><circle cx="12" cy="20" r="1" fill="currentColor" stroke="none"/></symbol>
<symbol id="i-chevron" viewBox="0 0 24 24"><path d="m9 5 7 7-7 7"/></symbol>
<symbol id="i-gear" viewBox="0 0 24 24"><path d="m9 3 1-1h4l1 3 3 1 3-1 1 4-2 2v3l2 2-2 4-3-1-2 1-1 3h-4l-1-3-3-1-3 1-2-4 2-2v-3L1 9l2-4 3 1 3-1Z" transform="translate(2 1) scale(.83)"/><circle cx="12" cy="12" r="3"/></symbol>
<symbol id="i-bulb" viewBox="0 0 24 24"><path d="M8 16c0-3-3-3-3-7a7 7 0 0 1 14 0c0 4-3 4-3 7Z" fill="currentColor" stroke="none"/><path d="M10 9v7m4-7v7" stroke="#fff4b1" stroke-width="1.2"/><path d="M9 18h6m-5 3h4"/></symbol>
<symbol id="i-fan" viewBox="0 0 24 24"><path d="M11 10C4 11 3 7 5 4c2-3 7-2 7 2v4m2 1c-1-7 3-8 6-6 3 2 2 7-2 7h-4m-1 2c7-1 8 3 6 6-2 3-7 2-7-2v-4m-2-1c1 7-3 8-6 6-3-2-2-7 2-7h4" fill="currentColor" stroke="none"/><circle cx="12" cy="12" r="2.5" fill="#a5ceff" stroke="none"/></symbol>
<symbol id="i-outlet" viewBox="0 0 24 24"><rect x="5" y="3" width="14" height="18" rx="3" fill="#fff"/><path d="M9 8v3m6-3v3m-4 5h2"/></symbol>
<symbol id="i-lamp" viewBox="0 0 24 24"><path d="M12 1v6m-3 0h6l1 5 4 5H4l4-5Z" fill="currentColor"/><path d="M9 19a3 3 0 0 0 6 0" stroke="#efc232" stroke-width="3"/></symbol>
<symbol id="i-power" viewBox="0 0 24 24"><path d="M12 2v10m-5-7a8 8 0 1 0 10 0"/></symbol>
<symbol id="i-clock" viewBox="0 0 24 24"><circle cx="12" cy="12" r="9"/><path d="M12 6v6l4 3"/></symbol>
<symbol id="i-calendar" viewBox="0 0 24 24"><rect x="4" y="5" width="16" height="16" rx="2"/><path d="M8 2v6m8-6v6M4 11h16"/></symbol>
<symbol id="i-home" viewBox="0 0 24 24"><path d="m3 10 9-8 9 8v11h-6v-8H9v8H3Z"/></symbol>
<symbol id="i-more" viewBox="0 0 24 24"><circle cx="12" cy="5" r="1.5" fill="currentColor" stroke="none"/><circle cx="12" cy="12" r="1.5" fill="currentColor" stroke="none"/><circle cx="12" cy="19" r="1.5" fill="currentColor" stroke="none"/></symbol>
<symbol id="i-plus" viewBox="0 0 24 24"><path d="M12 4v16M4 12h16"/></symbol>
<symbol id="i-close" viewBox="0 0 24 24"><path d="m6 6 12 12M6 18 18 6"/></symbol>
</defs></svg>
<header class="app-header"><h1>Điều khiển Relay 4 Kênh</h1><button id="settingsShortcut" class="icon-button" aria-label="Mở cài đặt"><svg class="icon " aria-hidden="true"><use href="#i-gear"/></svg></button></header>
<button class="device-strip" id="deviceShortcut" aria-label="Xem thông tin thiết bị"><span class="chip-icon"><svg class="icon " aria-hidden="true"><use href="#i-chip"/></svg></span><span class="device-copy"><span class="device-name">Thiết bị: <strong>ESP8266 Relay</strong></span><span id="connectionLine" class="connection-line">Trạng thái: <span id="connection" role="status">Đang kết nối</span></span></span><span class="ip-pill"><svg class="icon small" aria-hidden="true"><use href="#i-wifi"/></svg><span id="host">192.168.4.1</span></span><svg class="icon chevron" aria-hidden="true"><use href="#i-chevron"/></svg></button>
<div id="message" class="notice" role="status" aria-live="polite" hidden></div><div id="warning" class="notice" role="status" hidden></div>
<section id="relaySection" aria-label="Điều khiển 4 kênh"><div id="relays" class="grid"></div></section>
<section id="scheduleSection" aria-label="Lịch hẹn"><div class="sectionhead"><div class="section-title"><svg class="icon " aria-hidden="true"><use href="#i-clock"/></svg><h2>Lịch hẹn<span id="count" class="count">0/10</span></h2></div><button id="viewAll" class="see-all">Quản lý<svg class="icon " aria-hidden="true"><use href="#i-chevron"/></svg></button></div><div id="schedules" role="region" aria-label="Danh sách lịch hẹn, cuộn để xem tất cả" tabindex="0"><div class="empty">Đang tải lịch…</div></div><p id="scheduleScrollHint" class="scroll-hint" hidden>Vuốt hoặc cuộn lên/xuống để xem các lịch</p><button id="add" class="add-schedule" disabled><span class="add-symbol"><svg class="icon " aria-hidden="true"><use href="#i-plus"/></svg></span><span class="add-copy"><strong>Thêm lịch</strong><small>Tạo lịch hẹn mới cho các kênh</small></span><svg class="icon chevron" aria-hidden="true"><use href="#i-chevron"/></svg></button><p id="limitNote" class="limit-note" hidden>Đã đủ 10 lịch. Hãy sửa hoặc xóa lịch cũ.</p><p id="scheduleNote" class="note" hidden>Lịch lặp theo tuần. Kênh bật nếu có ít nhất một lịch đang chạy. Thay đổi lịch sẽ đưa cả 4 kênh về chế độ tự động.</p></section>
<section id="deviceSection" hidden><h2 class="page-heading">Thiết bị của bạn</h2><div class="info-card"><h3>ESP8266 NodeMCU</h3><div class="info-row"><span>Địa chỉ truy cập</span><strong id="deviceAddress"></strong></div><div class="info-row"><span>Môi trường</span><strong id="deviceMode">Wi-Fi riêng · AP</strong></div><div class="info-row"><span>Số kênh</span><strong>4 relay</strong></div><div class="info-row"><span>Chân điều khiển</span><strong>D1 · D2 · D5 · D6</strong></div><div class="info-row"><span>Mức kích mặc định</span><strong>Active LOW</strong></div><div class="info-row"><span>Bộ nhớ lịch</span><strong id="storageStatus">Đang kiểm tra</strong></div></div><p class="note">Kênh 1 đến 4 dùng biểu tượng đèn, quạt, ổ cắm và đèn trần để dễ phân biệt. Bạn có thể nối tải phù hợp với từng relay.</p></section>
<section id="settingsSection" hidden><h2 class="page-heading">Cài đặt</h2><div class="info-card"><h3>Kết nối Wi-Fi nhà</h3><p class="note">Nhập mạng 2.4 GHz để đồng bộ Firebase. Wi-Fi riêng của ESP vẫn dùng được.</p><form id="wifiForm"><div class="name-fields"><label class="name-field" for="wifiSsid"><span>Tên Wi-Fi (SSID)</span><input id="wifiSsid" required maxlength="32" autocomplete="off"></label><label class="name-field" for="wifiPassword"><span>Mật khẩu Wi-Fi</span><input id="wifiPassword" type="password" maxlength="63" autocomplete="new-password" placeholder="Để trống nếu mạng mở"></label></div><p id="wifiStatus" class="note" role="status">Đang kiểm tra Wi-Fi…</p><p id="firebaseStatus" class="note" role="status"></p><div class="dialogactions"><button id="connectWifi" type="submit" class="primary">Lưu &amp; kết nối</button></div></form></div><div class="info-card"><h3>Tên thiết bị</h3><p class="muted">Đặt tên riêng để dễ nhận biết từng kênh.</p><form id="namesForm"><div class="name-fields"><label class="name-field" for="name1"><span>Kênh 1</span><input id="name1" type="text" maxlength="48" required autocomplete="off" placeholder="Kênh 1"></label><label class="name-field" for="name2"><span>Kênh 2</span><input id="name2" type="text" maxlength="48" required autocomplete="off" placeholder="Kênh 2"></label><label class="name-field" for="name3"><span>Kênh 3</span><input id="name3" type="text" maxlength="48" required autocomplete="off" placeholder="Kênh 3"></label><label class="name-field" for="name4"><span>Kênh 4</span><input id="name4" type="text" maxlength="48" required autocomplete="off" placeholder="Kênh 4"></label></div><p class="note name-hint">Mỗi tên từ 1–24 ký tự, hỗ trợ tiếng Việt. Tên được giữ lại sau khi mất điện.</p><p id="namesError" class="notice error" role="alert" hidden></p><div class="dialogactions"><button id="saveNames" class="primary" type="submit" disabled>Lưu tên thiết bị</button></div></form></div><div class="info-card"><h3>Đồng hồ thiết bị</h3><div id="clock" class="clock">Chưa có giờ</div><div id="date" class="muted">Tự đồng bộ khi mở trang</div><button id="sync" class="primary">Đồng bộ giờ</button><p class="note">Giờ và múi giờ được lấy tự động từ điện thoại khi mở trang. Sau khi ESP mất điện, hãy mở lại trang để lấy giờ.</p></div><div class="info-card"><h3>Điều khiển &amp; lịch hẹn</h3><p class="note">Bật/tắt tay giữ hiệu lực đến khi trạng thái tổng hợp của lịch trên kênh đó thay đổi. Thay đổi lịch sẽ đưa các kênh về chế độ tự động.</p><p class="note">Lịch qua nửa đêm kết thúc vào ngày kế tiếp. Khi nhiều lịch trùng nhau, kênh chỉ tắt khi không còn lịch nào yêu cầu bật.</p></div></section>

</main>
<nav class="bottom-nav" aria-label="Điều hướng chính"><button class="nav-item active" data-page="home" aria-current="page"><svg class="icon home-icon" aria-hidden="true"><use href="#i-home"/></svg><span>Trang chủ</span></button><button class="nav-item" data-page="schedules"><svg class="icon " aria-hidden="true"><use href="#i-calendar"/></svg><span>Lịch</span></button><button class="nav-item" data-page="device"><svg class="icon " aria-hidden="true"><use href="#i-chip"/></svg><span>Thiết bị</span></button><button class="nav-item" data-page="settings"><svg class="icon " aria-hidden="true"><use href="#i-gear"/></svg><span>Cài đặt</span></button></nav>
<dialog id="editor" aria-labelledby="editorTitle"><form id="scheduleForm"><div class="dialog-heading"><h2 id="editorTitle">Thêm lịch</h2><button id="cancel" type="button" class="icon-button" aria-label="Đóng cửa sổ lịch"><svg class="icon " aria-hidden="true"><use href="#i-close"/></svg></button></div><p class="muted">Hẹn giờ bật/tắt tự động cho các kênh.</p><fieldset><legend>Chọn kênh điều khiển</legend><div id="relayChoices" class="choices"></div></fieldset><div class="timeinputs"><label>Bật lúc<input id="start" type="time" step="60" required value="08:00"></label><label>Tắt lúc<input id="end" type="time" step="60" required value="09:00"></label></div><p id="overnight" class="note"></p><fieldset><legend>Lặp lại vào · ngày bắt đầu</legend><div id="dayChoices" class="choices"></div></fieldset><label class="choice"><input id="enabled" type="checkbox" checked>Kích hoạt lịch hẹn</label><p id="formError" class="notice error" role="alert" hidden></p><div class="dialogactions"><button id="cancelBottom" type="button" class="secondary">Hủy</button><button id="save" type="submit" class="primary">Lưu lịch</button></div></form></dialog>
<script>
'use strict';
const $=id=>document.getElementById(id);
const dayNames=['Thứ 2','Thứ 3','Thứ 4','Thứ 5','Thứ 6','Thứ 7','CN'];
let state=null,busy=false,requesting=false,editing=-1,clockAt=0,lastSync=0,online=false;
const pad=n=>String(n).padStart(2,'0');
const hm=m=>pad(Math.floor(m/60))+':'+pad(m%60);
const minutes=value=>{const [h,m]=value.split(':').map(Number);return h*60+m;};
function element(tag,text,className){const el=document.createElement(tag);if(text!==undefined)el.textContent=text;if(className)el.className=className;return el;}

const shortDays=['T2','T3','T4','T5','T6','T7','Chủ nhật'];
const channelIcons=['bulb','fan','outlet','lamp'];
let page='home',openMenu=-1,messageTimer,namesDirty=false,lastScheduleRender='',lastRelayRender='';
function icon(name,className=''){const svg=document.createElementNS('http://www.w3.org/2000/svg','svg');svg.setAttribute('class','icon '+className);svg.setAttribute('aria-hidden','true');const use=document.createElementNS('http://www.w3.org/2000/svg','use');use.setAttribute('href','#i-'+name);svg.append(use);return svg;}
function relayName(id){return state?.relays.find(relay=>relay.id===id)?.name||'Kênh '+id;}
function channelIcon(id){const el=element('span',undefined,'channel-icon kind-'+id);el.append(icon(channelIcons[id-1]));return el;}
function switchButton(on,label,handler){const button=element('button',undefined,'switch');button.type='button';button.setAttribute('role','switch');button.setAttribute('aria-checked',String(on));button.setAttribute('aria-label',label);button.onclick=handler;return button;}
function showPage(next){page=next;openMenu=-1;$('relaySection').hidden=page!=='home';$('scheduleSection').hidden=!['home','schedules'].includes(page);$('deviceSection').hidden=page!=='device';$('settingsSection').hidden=page!=='settings';$('viewAll').hidden=page!=='home';$('scheduleNote').hidden=page!=='schedules';document.querySelectorAll('[data-page]').forEach(button=>{const selected=button.dataset.page===page;button.classList.toggle('active',selected);if(selected)button.setAttribute('aria-current','page');else button.removeAttribute('aria-current');});if(state)renderSchedules();}

function message(text,error=false){clearTimeout(messageTimer);$('message').textContent=text;$('message').className='notice '+(error?'error':'success');$('message').hidden=false;if(!error)messageTimer=setTimeout(()=>{$('message').hidden=true;},4500);}
function connection(ok){online=ok;$('connection').textContent=ok?'Đã kết nối':'Mất kết nối';$('connectionLine').className='connection-line'+(ok?' live':'');lock();}
function lock(){$('connectWifi').disabled=busy||requesting||!online||!state?.wifi;const waiting=busy||requesting;document.querySelectorAll('#relays button,#schedules button').forEach(b=>{b.disabled=waiting||!online;});$('add').disabled=waiting||!online||!state||!state.storageReady||state.schedules.length>=10;$('sync').disabled=waiting;$('save').disabled=waiting||!online;$('saveNames').disabled=waiting||!online||!state?.storageReady;}
async function api(path,data){
 const controller=new AbortController();const timer=setTimeout(()=>controller.abort(),8000);
 try{const response=await fetch(path,{method:data===undefined?'GET':'POST',cache:'no-store',headers:data===undefined?{}:{'Content-Type':'application/json'},body:data===undefined?undefined:JSON.stringify(data),signal:controller.signal});const result=await response.json();if(!response.ok)throw new Error(result.error||'Yêu cầu thất bại.');return result;}
 catch(error){if(error.name==='AbortError'||error instanceof TypeError||error instanceof SyntaxError){connection(false);throw new Error('Không nhận được phản hồi hợp lệ. Hãy kiểm tra Wi-Fi ESP8266 và tải lại trạng thái trước khi thử lại.');}throw error;}
 finally{clearTimeout(timer);}
}
function accept(next){
 const integer=(value,min,max)=>Number.isInteger(value)&&value>=min&&value<=max;
 const valid=next&&typeof next.timeValid==='boolean'&&Number.isFinite(next.epoch)&&next.epoch>=0&&integer(next.utcOffsetMinutes,-720,840)&&
  typeof next.storageReady==='boolean'&&typeof next.configHealthy==='boolean'&&Array.isArray(next.relays)&&next.relays.length===4&&
  next.relays.every(r=>r&&integer(r.id,1,4)&&typeof r.on==='boolean')&&new Set(next.relays.map(r=>r.id)).size===4&&
  Array.isArray(next.schedules)&&next.schedules.length<=10&&next.schedules.every(s=>s&&integer(s.id,0,9)&&typeof s.enabled==='boolean'&&integer(s.relays,1,15)&&integer(s.days,1,127)&&integer(s.start,0,1439)&&integer(s.end,0,1439)&&s.start!==s.end)&&new Set(next.schedules.map(s=>s.id)).size===next.schedules.length;
 if(!valid){connection(false);throw new Error('Trạng thái thiết bị không hợp lệ. Hãy tải lại trạng thái trước khi điều khiển.');}
 state=next;clockAt=performance.now();connection(true);render();
}
async function mutate(path,data,success){
 if(busy||requesting)return false;busy=true;lock();
 try{accept(await api(path,data));if(success)message(success);return true;}
 catch(error){message(error.message,true);return false;}
 finally{busy=false;lock();}
}
function paintClock(){
 if(!state||!state.timeValid){$('clock').textContent='Chưa có giờ';$('date').textContent='Mở trang để đồng bộ từ điện thoại';return;}
 const elapsed=Math.floor((performance.now()-clockAt)/1000);
 const d=new Date((state.epoch+elapsed+state.utcOffsetMinutes*60)*1000);
 $('clock').textContent=pad(d.getUTCHours())+':'+pad(d.getUTCMinutes())+':'+pad(d.getUTCSeconds());
 const offset=state.utcOffsetMinutes;const zone='UTC'+(offset>=0?'+':'−')+hm(Math.abs(offset));
 $('date').textContent=dayNames[(d.getUTCDay()+6)%7]+', '+pad(d.getUTCDate())+'/'+pad(d.getUTCMonth()+1)+'/'+d.getUTCFullYear()+' · '+zone+(online?'':' · Ước tính');
}
function renderRelays(){
 const signature=JSON.stringify(state.relays);
 if(signature===lastRelayRender)return;
 lastRelayRender=signature;
 $('relays').replaceChildren();
 state.relays.forEach(r=>{
  const card=element('article',undefined,'relay'+(r.on?' on':''));card.setAttribute('aria-label',relayName(r.id));
  const top=element('div',undefined,'relaytop');const copy=element('div',undefined,'relay-copy');copy.append(element('h3',relayName(r.id)),element('span',r.on?'Đang bật':'Đang tắt','relay-status'));
  top.append(channelIcon(r.id),copy,switchButton(r.on,'Bật/tắt '+relayName(r.id),()=>mutate('/api/relay',{relay:r.id,on:!r.on})));card.append(top);
  const buttons=element('div',undefined,'relay-buttons');
  [true,false].forEach(on=>{const button=element('button',undefined,'power-button'+(r.on===on?(on?' selected-on':' selected-off'):''));button.setAttribute('aria-label',(on?'Bật':'Tắt')+' '+relayName(r.id));button.setAttribute('aria-pressed',String(r.on===on));button.append(icon('power'),document.createTextNode(on?'Bật':'Tắt'));button.onclick=()=>mutate('/api/relay',{relay:r.id,on});buttons.append(button);});card.append(buttons);
  $('relays').append(card);
 });
}
function renderSchedules(){
 $('count').textContent=state.schedules.length+'/10';$('limitNote').hidden=state.schedules.length<10;
 $('scheduleScrollHint').hidden=state.schedules.length<4;
 // Keep the list DOM while polling unchanged data so touch scrolling continues.
 const signature=JSON.stringify([state.schedules,state.relays.map(r=>r.name),openMenu]);
 if(signature===lastScheduleRender){lock();return;}
 lastScheduleRender=signature;
 const focused=document.activeElement?.dataset?.focus;
 const scrollTop=$('schedules').scrollTop;
 $('schedules').replaceChildren();
 if(!state.schedules.length){const empty=element('div',undefined,'empty');empty.append(icon('calendar'),element('p','Chưa có lịch hẹn'),element('p','Thêm lịch để tự động bật/tắt các kênh.','note'));$('schedules').append(empty);}
 state.schedules.forEach(s=>{
  const channels=[1,2,3,4].filter(n=>s.relays&(1<<(n-1)));
  const card=element('article',undefined,'schedule'+(s.enabled?'':' disabled'));const row=element('div',undefined,'schedule-row');
  const edit=element('button',undefined,'schedule-edit');edit.dataset.focus='edit-'+s.id;edit.setAttribute('aria-label','Sửa lịch '+(s.id+1));edit.onclick=()=>openEditor(s);
  edit.append(element('h3',channels.map(relayName).join(', ')+' · '+hm(s.start)+' – '+hm(s.end)+(s.start>s.end?' (+1 ngày)':'')));
  const days=element('span',undefined,'schedule-days');days.append(icon('calendar'),document.createTextNode(s.days===127?'Hằng ngày':shortDays.filter((_,i)=>s.days&(1<<i)).join(', ')));edit.append(days);
  const toggle=switchButton(s.enabled,'Kích hoạt lịch '+(s.id+1),()=>mutate('/api/schedule',{...s,enabled:!s.enabled},'Đã cập nhật lịch; các kênh trở về tự động.'));toggle.dataset.focus='toggle-'+s.id;
  const more=element('button',undefined,'schedule-more');more.append(icon('more'));more.setAttribute('aria-label','Tùy chọn lịch '+(s.id+1));more.setAttribute('aria-expanded',String(openMenu===s.id));more.dataset.focus='more-'+s.id;more.onclick=()=>{openMenu=openMenu===s.id?-1:s.id;renderSchedules();};
  row.append(channelIcon(channels[0]),edit,toggle,more);card.append(row);
  const actions=element('div',undefined,'actions');actions.hidden=openMenu!==s.id;
  const editAction=element('button','Sửa lịch');editAction.dataset.action='edit';editAction.onclick=()=>openEditor(s);
  const del=element('button','Xóa lịch','danger');del.dataset.action='delete';del.onclick=()=>{if(confirm('Xóa lịch '+(s.id+1)+'?')){openMenu=-1;mutate('/api/schedule/delete',{id:s.id},'Đã xóa lịch; các kênh trở về tự động.');}};
  actions.append(editAction,del);card.append(actions);$('schedules').append(card);
 });
 $('schedules').scrollTop=scrollTop;
 lock();
 if(focused){const target=[...$('schedules').querySelectorAll('[data-focus]')].find(el=>el.dataset.focus===focused);if(target&&!target.disabled)target.focus({preventScroll:true});}
}
function render(){
 if(state.wifi){$('wifiStatus').textContent=state.wifi.status+(state.wifi.ip?' · '+state.wifi.ip:'');$('firebaseStatus').textContent=state.wifi.firebase;if(!$('wifiSsid').value)$('wifiSsid').value=state.wifi.ssid||'';}
 else{$('wifiStatus').textContent='Chỉ cấu hình Wi-Fi trên ESP thật.';}

 paintClock();
 const warnings=[];
 if(!state.timeValid)warnings.push('Thiết bị chưa có giờ: lịch đang tạm dừng. Vào Cài đặt để đồng bộ giờ.');
 if(!state.storageReady)warnings.push('Không truy cập được bộ nhớ flash. Không thể lưu lịch.');
 else if(!state.configHealthy)warnings.push('Lỗi cấu hình flash. Kiểm tra các lịch và lưu lại trước khi sử dụng.');
 $('warning').textContent=warnings.join(' ');$('warning').hidden=warnings.length===0;
 $('storageStatus').textContent=state.storageReady&&state.configHealthy?'Sẵn sàng · '+state.schedules.length+'/10 lịch':'Cần kiểm tra';
 
 renderNameInputs();renderRelays();renderSchedules();
}
function choices(target,labels,mask){$(target).replaceChildren();labels.forEach((name,i)=>{const label=element('label',undefined,'choice');const input=element('input');input.type='checkbox';input.value=1<<i;input.checked=!!(mask&(1<<i));label.append(input,document.createTextNode(name));$(target).append(label);});}
function mask(target){return [...$(target).querySelectorAll('input:checked')].reduce((value,input)=>value|Number(input.value),0);}
function overnight(){const a=minutes($('start').value),b=minutes($('end').value);$('overnight').textContent=a>b?'Qua nửa đêm: tắt vào ngày kế tiếp. Thứ được chọn là ngày bắt đầu.':a===b?'Giờ bật và giờ tắt phải khác nhau.':'Bật và tắt trong cùng ngày.';}
function openEditor(s){editing=s?s.id:-1;$('editorTitle').textContent=s?'Sửa lịch '+(s.id+1):'Thêm lịch';choices('relayChoices',[1,2,3,4].map(id=>'Kênh '+id+' · '+relayName(id)),s?s.relays:1);choices('dayChoices',shortDays,s?s.days:127);$('start').value=hm(s?s.start:480);$('end').value=hm(s?s.end:540);$('enabled').checked=s?s.enabled:true;$('formError').hidden=true;overnight();$('editor').showModal();}
$('add').onclick=()=>openEditor(null);$('cancel').onclick=$('cancelBottom').onclick=()=>$('editor').close();$('start').oninput=overnight;$('end').oninput=overnight;
$('scheduleForm').onsubmit=async event=>{
 event.preventDefault();const data={id:editing,enabled:$('enabled').checked,relays:mask('relayChoices'),days:mask('dayChoices'),start:minutes($('start').value),end:minutes($('end').value)};
 if(!data.relays||!data.days||!Number.isInteger(data.start)||!Number.isInteger(data.end)||data.start===data.end){$('formError').textContent='Chọn ít nhất một relay, một thứ và hai giờ khác nhau.';$('formError').hidden=false;return;}
 if(await mutate('/api/schedule',data,'Đã lưu lịch; các relay trở về theo lịch.'))$('editor').close();
 else{$('formError').textContent=$('message').textContent||'Đang tải trạng thái, vui lòng thử lại.';$('formError').hidden=false;}
};
async function syncTime(silent=false){
 const ok=await mutate('/api/time',{epoch:Math.floor(Date.now()/1000),utcOffsetMinutes:-new Date().getTimezoneOffset()},silent?'':'Đã đồng bộ giờ từ điện thoại.');if(ok)lastSync=Date.now();return ok;
}
$('sync').onclick=()=>syncTime();
async function refresh(){
 if(document.hidden||busy||requesting)return;requesting=true;lock();
 try{accept(await api('/api/state'));}catch(error){connection(false);message(error.message,true);}finally{requesting=false;lock();}
 if(state&&online&&(!state.timeValid||Date.now()-lastSync>300000))await syncTime(true);
}

function renderNameInputs(){if(!namesDirty)[1,2,3,4].forEach(id=>{$('name'+id).value=relayName(id);});}
[1,2,3,4].forEach(id=>{$('name'+id).addEventListener('input',()=>{namesDirty=true;$('namesError').hidden=true;});});
$('namesForm').onsubmit=async event=>{
 event.preventDefault();
 if(busy||requesting)return;
 const names=[1,2,3,4].map(id=>$('name'+id).value.trim());
 if(names.some(name=>!name||[...name].length>24||/[\u0000-\u001f\u007f-\u009f\u2028\u2029]/u.test(name))){$('namesError').textContent='Nhập đủ 4 tên, mỗi tên 1–24 ký tự, không chứa ký tự điều khiển.';$('namesError').hidden=false;return;}
 [1,2,3,4].forEach(id=>{$('name'+id).disabled=true;});
 const saved=await mutate('/api/names',{names},'Đã lưu tên 4 thiết bị.');
 [1,2,3,4].forEach(id=>{$('name'+id).disabled=false;});
 if(saved){namesDirty=false;renderNameInputs();$('namesError').hidden=true;}
 else{$('namesError').textContent=$('message').textContent;$('namesError').hidden=false;}
};

$('wifiForm').onsubmit=async event=>{
 event.preventDefault();
 const ssid=$('wifiSsid').value,password=$('wifiPassword').value;
 const size=value=>new TextEncoder().encode(value).length;
 if(size(ssid)>32||(password.length>0&&(size(password)<8||size(password)>63))){message('SSID tối đa 32 byte; mật khẩu 8–63 byte hoặc để trống.',true);return;}
 if(await mutate('/api/wifi',{ssid,password},'Đã lưu Wi-Fi. Đang kết nối; nếu mất trang, kết nối lại Wi-Fi ESP.'))$('wifiPassword').value='';
};
$('host').textContent=location.host||'192.168.4.1';$('deviceAddress').textContent=location.host||'192.168.4.1';
document.querySelectorAll('[data-page]').forEach(button=>{button.onclick=()=>showPage(button.dataset.page);});
$('viewAll').onclick=()=>showPage('schedules');$('settingsShortcut').onclick=()=>showPage('settings');$('deviceShortcut').onclick=()=>showPage('device');
document.addEventListener('keydown',event=>{if(event.key==='Escape'&&openMenu!==-1){openMenu=-1;renderSchedules();}});
document.addEventListener('visibilitychange',()=>{if(!document.hidden)refresh();});
setInterval(paintClock,1000);setInterval(refresh,3000);refresh();
</script>
</body></html>)HTML";
