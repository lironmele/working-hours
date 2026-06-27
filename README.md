# ⏱ Working Hours Tracker

A small static site for tracking working hours: **clock in**, **clock out**, and a **history** view.

This is a mock / prototype. Clock in and clock out work entirely in the browser — they update the on-screen state but **do not persist** any data. Refreshing the page resets to the seed data.

## Features

- Live clock with current date
- Clock In / Clock Out with an active-session timer
- History table (date, in, out, total, note)
- Summary stats: this week's total, average per day, days logged
- Seed data loaded from a mock JSON "database"

## Project structure

```
.
├── index.html      # Page markup
├── styles.css      # Styling
├── app.js          # Frontend logic (clock + rendering)
├── data/
│   └── db.json     # Mock database with fake history
└── README.md
```

## Running locally

Because the app fetches `data/db.json`, serve it over HTTP rather than opening
the file directly:

```bash
python3 -m http.server 8000
# then open http://localhost:8000
```

(If opened via `file://`, the page still renders but starts with empty history.)

## Deployment

This site is deployed with **GitHub Pages**. It's a plain static site, so no
build step is required — Pages can serve the repository root directly.

## Notes

- `data/db.json` holds fake data for demo purposes.
- Clock in/out is frontend-only; there is no backend and nothing is saved.
