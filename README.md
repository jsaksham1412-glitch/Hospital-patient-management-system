# Hospital Patient Management System

A small C++17 hospital workflow model with a browser dashboard and a console mode. Patient records are held in memory and are cleared when the server exits.

## Data structures

- **List:** Stores registered patient records and supports listing/searching them.
- **Queue:** Holds patients waiting for triage; the next patient is admitted in FIFO order.
- **Binary search tree:** Indexes patient IDs and prints them in sorted order with an in-order traversal.
- **Graph:** Models hospital locations. Breadth-first search finds a route between locations.

## Build and run

```sh
cmake -S . -B build
cmake --build build -j2
./build/hospital
```

Open `http://127.0.0.1:8080` to use the dashboard. CMake fetches `cpp-httplib` and `nlohmann/json` during configuration, so the first build needs an internet connection. To use the original text menu instead, run `./build/hospital --console`.

Register a patient to receive an ID, starting at `1001`. The dashboard supports patient search, FIFO triage admission, discharge, and route-finding through the hospital map.

This is an educational model, not a clinical or production patient-record system.