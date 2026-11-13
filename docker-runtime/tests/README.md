# Web UI verification

The production UI in `docker-runtime/web/` uses native JavaScript modules and
needs no frontend build or package installation. The runtime image copies all
web assets, including `buffer-model.js`.

Run the buffer boundary and marker collision tests with Node.js 22 or later:

```sh
node --test docker-runtime/tests/buffer_model.test.mjs
```

Run the browser checks with Playwright and its Chromium browser installed:

```sh
node --test docker-runtime/tests/web_ui.test.mjs
```

`PLAYWRIGHT_MODULE` can point to an existing Playwright `index.mjs` and
`CHROMIUM_PATH` can select an installed Chrome/Chromium executable.
Screenshots go to the system temporary directory, or `UI_SCREENSHOT_DIR`.

The browser suite serves the actual production HTML, CSS and JavaScript with
nginx's Content Security Policy. All API calls use test responses; it never
connects to a real delayer or database. It checks authentication, command and
configuration forms, UTC payloads, expired-position recovery, both buffer views,
selection and focus across polling, responsive layouts from 320 to 1440 pixels,
stale/disconnected status, untrusted text, and session expiration.

For real Linux builds, multicast playback, chunk analysis, authentication and
configuration persistence, run the isolated Docker suite:

```sh
python3 docker-runtime/tests/run.py
```
