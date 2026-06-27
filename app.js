// Working Hours Tracker — static, frontend-only mock.
// Clock in/out updates the in-memory state only; nothing is persisted.

const state = {
  user: null,
  entries: [],      // from mock db + any added this session
  activeSince: null // Date when currently clocked in, or null
};

// ---- Helpers ----------------------------------------------------------

const $ = (id) => document.getElementById(id);

const pad = (n) => String(n).padStart(2, "0");

function fmtClock(d) {
  return `${pad(d.getHours())}:${pad(d.getMinutes())}:${pad(d.getSeconds())}`;
}

function fmtDateLong(d) {
  return d.toLocaleDateString(undefined, {
    weekday: "long", year: "numeric", month: "long", day: "numeric"
  });
}

// minutes between "HH:MM" strings
function minutesBetween(start, end) {
  const [sh, sm] = start.split(":").map(Number);
  const [eh, em] = end.split(":").map(Number);
  return (eh * 60 + em) - (sh * 60 + sm);
}

function fmtDuration(mins) {
  const h = Math.floor(mins / 60);
  const m = mins % 60;
  return `${h}h ${m}m`;
}

function fmtElapsed(ms) {
  const total = Math.floor(ms / 1000);
  const h = Math.floor(total / 3600);
  const m = Math.floor((total % 3600) / 60);
  const s = total % 60;
  return `${pad(h)}:${pad(m)}:${pad(s)}`;
}

// ISO week key so "this week" groups Mon–Sun
function weekKey(dateStr) {
  const d = new Date(dateStr + "T00:00:00");
  const day = (d.getDay() + 6) % 7; // Mon=0
  d.setDate(d.getDate() - day);
  return d.toISOString().slice(0, 10);
}

// ---- Rendering --------------------------------------------------------

function renderUser() {
  const u = state.user;
  $("userName").textContent = u.name;
  $("userEmail").textContent = u.email;
  $("avatar").textContent = (u.name || "?").trim().charAt(0).toUpperCase();
  $("weekTarget").textContent = `of ${u.weeklyTarget}h target`;
}

function renderHistory() {
  const body = $("historyBody");
  body.innerHTML = "";

  const sorted = [...state.entries].sort((a, b) => b.date.localeCompare(a.date));

  for (const e of sorted) {
    const tr = document.createElement("tr");
    if (e._new) tr.className = "row-new";

    const open = !e.clockOut;
    const totalCell = open
      ? `<span class="pill">In progress</span>`
      : fmtDuration(minutesBetween(e.clockIn, e.clockOut));

    tr.innerHTML = `
      <td>${e.date}</td>
      <td>${e.clockIn}</td>
      <td>${e.clockOut || "—"}</td>
      <td>${totalCell}</td>
      <td class="note-cell">${e.note || ""}</td>
    `;
    body.appendChild(tr);
  }

  $("historyCount").textContent =
    `${state.entries.length} ${state.entries.length === 1 ? "entry" : "entries"}`;
}

function renderStats() {
  const completed = state.entries.filter((e) => e.clockOut);

  const thisWeek = weekKey(new Date().toISOString().slice(0, 10));
  const weekMins = completed
    .filter((e) => weekKey(e.date) === thisWeek)
    .reduce((sum, e) => sum + minutesBetween(e.clockIn, e.clockOut), 0);
  $("weekTotal").textContent = fmtDuration(weekMins);

  const totalMins = completed.reduce(
    (sum, e) => sum + minutesBetween(e.clockIn, e.clockOut), 0);
  const avg = completed.length ? Math.round(totalMins / completed.length) : 0;
  $("avgDay").textContent = fmtDuration(avg);

  $("daysLogged").textContent = String(state.entries.length);
}

function renderClockState() {
  const active = !!state.activeSince;
  $("statusDot").classList.toggle("on", active);
  $("statusText").textContent = active ? "Clocked in" : "Clocked out";
  $("clockInBtn").disabled = active;
  $("clockOutBtn").disabled = !active;

  const info = $("sessionInfo");
  if (active) {
    info.hidden = false;
    $("sessionStart").textContent = fmtClock(state.activeSince).slice(0, 5);
  } else {
    info.hidden = true;
  }
}

// ---- Live clock -------------------------------------------------------

function tick() {
  const now = new Date();
  $("liveClock").textContent = fmtClock(now);
  $("liveDate").textContent = fmtDateLong(now);
  if (state.activeSince) {
    $("sessionElapsed").textContent = fmtElapsed(now - state.activeSince);
  }
}

// ---- Actions ----------------------------------------------------------

function clockIn() {
  if (state.activeSince) return;
  state.activeSince = new Date();
  renderClockState();
  tick();
}

function clockOut() {
  if (!state.activeSince) return;
  const start = state.activeSince;
  const end = new Date();
  state.activeSince = null;

  // Mark any prior "new" rows as old so only this one flashes.
  state.entries.forEach((e) => (e._new = false));

  state.entries.push({
    id: Date.now(),
    date: end.toISOString().slice(0, 10),
    clockIn: fmtClock(start).slice(0, 5),
    clockOut: fmtClock(end).slice(0, 5),
    note: "Logged this session",
    _new: true
  });

  renderClockState();
  renderHistory();
  renderStats();
}

// ---- Init -------------------------------------------------------------

async function init() {
  $("clockInBtn").addEventListener("click", clockIn);
  $("clockOutBtn").addEventListener("click", clockOut);

  try {
    const res = await fetch("data/db.json");
    const db = await res.json();
    state.user = db.user;
    state.entries = db.entries.map((e) => ({ ...e }));
  } catch (err) {
    // Fallback so the page still renders if fetch is blocked (e.g. file://)
    console.error("Could not load mock db, using fallback.", err);
    state.user = { name: "Demo User", email: "demo@example.com", weeklyTarget: 40 };
    state.entries = [];
  }

  renderUser();
  renderClockState();
  renderHistory();
  renderStats();

  tick();
  setInterval(tick, 1000);
}

init();
