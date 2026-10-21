const api = async (path, options = {}) => {
  const response = await fetch(path, {
    ...options,
    headers: { "Content-Type": "application/json", ...options.headers },
  });
  const body = await response.json();
  if (!response.ok) throw new Error(body.error || "Request could not be completed.");
  return body;
};

const state = { patients: [], toastTimer: null };
const byId = (id) => document.getElementById(id);
const escapeHtml = (value) => String(value).replace(/[&<>"']/g, (character) => ({
  "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;",
}[character]));

function notify(message, isError = false) {
  const toast = byId("toast");
  toast.textContent = message;
  toast.classList.toggle("error", isError);
  toast.classList.add("visible");
  clearTimeout(state.toastTimer);
  state.toastTimer = setTimeout(() => toast.classList.remove("visible"), 2800);
}

function initials(name) {
  return name.trim().split(/\s+/).slice(0, 2).map((part) => part[0] || "").join("").toUpperCase();
}

function statusClass(status) {
  if (status === "Admitted") return "status-admitted";
  if (status === "Discharged") return "status-discharged";
  return "status-waiting";
}

function renderPatients() {
  const query = byId("patient-search").value.trim().toLowerCase();
  const filter = byId("status-filter").value;
  const matching = state.patients.filter((patient) => {
    const text = `${patient.name} ${patient.id} ${patient.condition}`.toLowerCase();
    return text.includes(query) && (filter === "all" || patient.status === filter);
  });
  byId("patient-rows").innerHTML = matching.map((patient) => `
    <tr>
      <td><div class="patient-cell"><span class="patient-avatar">${escapeHtml(initials(patient.name))}</span><span><span class="patient-name">${escapeHtml(patient.name)}</span><span class="patient-id">#${patient.id}</span></span></div></td>
      <td>${patient.age}</td>
      <td class="patient-condition" title="${escapeHtml(patient.condition)}">${escapeHtml(patient.condition)}</td>
      <td><span class="status-badge ${statusClass(patient.status)}">${escapeHtml(patient.status === "Waiting for triage" ? "Waiting" : patient.status)}</span></td>
      <td>${patient.status === "Admitted" ? `<button class="row-action" type="button" data-discharge="${patient.id}" aria-label="Discharge ${escapeHtml(patient.name)}"><i data-lucide="log-out"></i> Discharge</button>` : ""}</td>
    </tr>`).join("");

  const noPatients = state.patients.length === 0;
  byId("patients-empty").classList.toggle("hidden", !noPatients);
  byId("search-empty").classList.toggle("hidden", noPatients || matching.length > 0);
  byId("patient-rows").classList.toggle("hidden", matching.length === 0);
  byId("directory-count").textContent = state.patients.length;
  byId("table-summary").textContent = `Showing ${matching.length} of ${state.patients.length} patients`;
  if (window.lucide) window.lucide.createIcons();
}

function renderQueue() {
  const waiting = state.patients.filter((patient) => patient.status === "Waiting for triage");
  byId("queue-heading-count").textContent = waiting.length;
  byId("nav-queue-count").textContent = waiting.length;
  byId("admit-next").disabled = waiting.length === 0;
  byId("queue-empty").classList.toggle("hidden", waiting.length > 0);
  byId("queue-list").classList.toggle("hidden", waiting.length === 0);
  byId("queue-list").innerHTML = waiting.slice(0, 4).map((patient, index) => `
    <div class="queue-row">
      <span class="queue-position">${String(index + 1).padStart(2, "0")}</span>
      <span class="queue-patient"><strong>${escapeHtml(patient.name)}</strong><span>${escapeHtml(patient.condition)} · ${patient.age} years</span></span>
      <span class="queue-id">#${patient.id}</span>
    </div>`).join("");
}

function renderMetrics(summary) {
  const discharged = state.patients.filter((patient) => patient.status === "Discharged").length;
  byId("metric-total").textContent = summary.total;
  byId("metric-waiting").textContent = summary.waiting;
  byId("metric-admitted").textContent = summary.admitted;
  byId("metric-discharged").textContent = discharged;
}

async function refresh() {
  const summary = await api("/api/patients");
  state.patients = summary.patients;
  renderMetrics(summary);
  renderQueue();
  renderPatients();
}

async function loadLocations() {
  const { locations } = await api("/api/locations");
  for (const selectId of ["route-from", "route-to"]) {
    byId(selectId).innerHTML = locations.map((location) => `<option value="${escapeHtml(location)}">${escapeHtml(location)}</option>`).join("");
  }
  byId("route-to").selectedIndex = locations.length - 1;
}

byId("registration-form").addEventListener("submit", async (event) => {
  event.preventDefault();
  const form = new FormData(event.currentTarget);
  try {
    const patient = await api("/api/patients", {
      method: "POST",
      body: JSON.stringify({ name: form.get("name").trim(), age: Number(form.get("age")), condition: form.get("condition").trim() }),
    });
    event.currentTarget.reset();
    await refresh();
    notify(`Patient #${patient.id} added to the triage queue.`);
    byId("patient-name").focus();
  } catch (error) {
    notify(error.message, true);
  }
});

byId("admit-next").addEventListener("click", async () => {
  try {
    const patient = await api("/api/triage/next", { method: "POST", body: "{}" });
    await refresh();
    notify(`${patient.name} admitted from the queue.`);
  } catch (error) {
    notify(error.message, true);
  }
});

byId("patient-rows").addEventListener("click", async (event) => {
  const button = event.target.closest("[data-discharge]");
  if (!button) return;
  try {
    const patient = await api(`/api/patients/${button.dataset.discharge}/discharge`, { method: "POST", body: "{}" });
    await refresh();
    notify(`${patient.name} discharged.`);
  } catch (error) {
    notify(error.message, true);
  }
});

byId("patient-search").addEventListener("input", renderPatients);
byId("status-filter").addEventListener("change", renderPatients);
byId("focus-registration").addEventListener("click", () => {
  byId("registration").scrollIntoView({ behavior: "smooth", block: "center" });
  byId("patient-name").focus({ preventScroll: true });
});

byId("route-form").addEventListener("submit", async (event) => {
  event.preventDefault();
  const from = byId("route-from").value;
  const to = byId("route-to").value;
  try {
    const result = await api(`/api/route?from=${encodeURIComponent(from)}&to=${encodeURIComponent(to)}`);
    byId("route-result").innerHTML = result.route.map((place) => `<span class="route-step">${escapeHtml(place)}</span>`).join("");
    byId("route-result").classList.remove("hidden");
  } catch (error) {
    notify(error.message, true);
  }
});

document.querySelectorAll(".nav-link").forEach((link) => link.addEventListener("click", () => {
  document.querySelectorAll(".nav-link").forEach((item) => item.classList.remove("active"));
  link.classList.add("active");
}));

byId("today-label").textContent = new Intl.DateTimeFormat(undefined, { weekday: "short", month: "short", day: "numeric", year: "numeric" }).format(new Date()).toUpperCase();
if (window.lucide) window.lucide.createIcons();
Promise.all([refresh(), loadLocations()]).catch((error) => notify(error.message, true));