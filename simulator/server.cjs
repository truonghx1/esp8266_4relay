'use strict';
// No npm dependencies. Serves the actual firmware HTML and a local API simulator.
const http = require('node:http');
const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const { Simulator, ApiError } = require('./model.cjs');
const root = path.resolve(__dirname, '..');

function createServer({ file = path.join(root, '.simulator', 'config.json') } = {}) {
  const model = new Simulator(file);
  model.testClock = false; // First opening behaves like the real firmware.
  function respond(res, status, body, type = 'application/json; charset=utf-8') {
    res.writeHead(status, { 'Content-Type': type, 'Cache-Control': 'no-store', 'X-Content-Type-Options': 'nosniff' });
    res.end(type.startsWith('application/json') ? JSON.stringify(body) : body);
  }
  const server = http.createServer(async (req, res) => {
    try {
      const route = new URL(req.url, 'http://localhost').pathname;
      if (req.method === 'GET' && route === '/') {
        const source = fs.readFileSync(path.join(root, 'firmware', 'esp8266_4relay', 'web_ui.h'), 'utf8');
        const html = source.match(/R"HTML\(([\s\S]*)\)HTML";/)?.[1];
        if (!html) throw Error('Cannot extract firmware HTML');
        const panel = fs.readFileSync(path.join(__dirname, 'panel.html'), 'utf8');
        return respond(res, 200, html.replace('</main>', panel + '</main>').replace('</body>', '<script src="/sim-panel.js"></script></body>'), 'text/html; charset=utf-8');
      }
      if (req.method === 'GET' && route === '/sim-panel.js')
        return respond(res, 200, fs.readFileSync(path.join(__dirname, 'panel.js'), 'utf8'), 'text/javascript; charset=utf-8');
      if (req.method === 'GET' && route === '/api/state') return respond(res, 200, model.state());
      if (req.method !== 'POST') throw new ApiError(404, 'Không tìm thấy đường dẫn.');
      if ((req.headers['content-type'] || '').split(';')[0].trim().toLowerCase() !== 'application/json')
        throw new ApiError(415, 'Yêu cầu Content-Type: application/json.');
      // Reject browser requests from a different site. No CORS is enabled.
      if (req.headers.origin) {
        let origin;
        try { origin = new URL(req.headers.origin).origin; } catch { /* Invalid origins are forbidden too. */ }
        if (origin !== `http://${req.headers.host}`)
          throw new ApiError(403, 'Nguồn yêu cầu không hợp lệ.');
      }
      const chunks = [];
      let length = 0;
      for await (const chunk of req) {
        length += chunk.length;
        if (length > 1024) throw new ApiError(413, 'Nội dung quá dài.');
        chunks.push(chunk);
      }
      let data;
      // TCP may split a Vietnamese character between chunks. Decode once after
      // assembling the bytes, and reject malformed UTF-8 instead of saving U+FFFD.
      try { data = JSON.parse(new TextDecoder('utf-8', { fatal: true }).decode(Buffer.concat(chunks, length))); }
      catch { throw new ApiError(400, 'JSON không hợp lệ.'); }
      if (!data || Array.isArray(data) || typeof data !== 'object') throw new ApiError(400, 'JSON phải là object.');
      respond(res, 200, model.command(route, data));
    } catch (error) {
      if (!(error instanceof ApiError)) console.error(error);
      if (!res.headersSent) respond(res, error.status || 500, { error: error.status ? error.message : 'Lỗi máy chủ mô phỏng.' });
    }
  });
  const timer = setInterval(() => model.evaluate(), 100);
  timer.unref();
  server.on('close', () => clearInterval(timer));
  server.requestTimeout = 10000;
  return { server, model };
}

if (require.main === module) {
  const port = Number(process.env.PORT || 8266);
  if (!Number.isInteger(port) || port < 1 || port > 65535) throw Error('PORT must be 1..65535');
  const lan = process.argv.includes('--lan');
  const { server } = createServer();
  server.on('error', error => {
    console.error(error.code === 'EADDRINUSE' ? `Port ${port} is already in use. Try http://localhost:${port} or choose another PORT.` : error);
    process.exitCode = 1;
  });
  server.listen(port, lan ? '0.0.0.0' : '127.0.0.1', () => {
    console.log(`ESP8266 simulator: http://localhost:${port}`);
    if (lan) for (const entries of Object.values(os.networkInterfaces())) for (const addr of entries || [])
      if (addr.family === 'IPv4' && !addr.internal) console.log(`Phone / LAN: http://${addr.address}:${port}`);
    console.log('No hardware required. Configuration: .simulator/config.json');
    console.log('Press Ctrl+C to stop.');
  });
}
module.exports = { createServer };
