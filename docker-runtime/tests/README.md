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
configuration forms, fixed yyyy-mm-dd dates and hh:mm:ss placeholders across
browser locales, 24-hour time validation, ticking UTC clocks beside both optional
time fields (without changing entered values), UTC payloads,
expired-position recovery, Recorded time validation before schedule checks
(including changing buffer bounds and unavailable status), scheduled buffer
expiry validation (including retention changes and blocked submissions), stop times
after scheduled starts, future wall-clock times (including open forms as time
passes), both buffer views,
selection and focus across polling, responsive layouts from 320 to 1440 pixels,
single-use Play/Pause arming, automatic disarming, stop confirmation,
steady yellow scheduled-start and green/red stop indicators (including reduced motion),
start/stop countdowns that tick each second and hide when not applicable,
stale/disconnected status, untrusted text, and session expiration.

For real Linux builds, multicast playback, chunk analysis, authentication and
configuration persistence, run the isolated Docker suite:

```sh
python3 docker-runtime/tests/run.py
```
