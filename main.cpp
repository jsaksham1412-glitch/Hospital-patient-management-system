run
#include <algorithm>
#include <iostream>
#include <list>
#include <memory>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "httplib.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

struct Patient {
    int id;
    std::string name;
    int age;
    std::string condition;
    std::string status;
};

class PatientIdTree {
private:
    struct Node {
        explicit Node(int patientId) : id(patientId) {}

        int id;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
    };

    std::unique_ptr<Node> root_;

    static bool insert(std::unique_ptr<Node>& node, int id) {
        if (!node) {
            node = std::make_unique<Node>(id);
            return true;
        }
        if (id == node->id) {
            return false;
        }
        return insert(id < node->id ? node->left : node->right, id);
    }

    static bool contains(const Node* node, int id) {
        if (!node) {
            return false;
        }
        if (id == node->id) {
            return true;
        }
        return contains(id < node->id ? node->left.get() : node->right.get(), id);
    }

    static void collectSorted(const Node* node, std::vector<int>& ids) {
        if (!node) {
            return;
        }
        collectSorted(node->left.get(), ids);
        ids.push_back(node->id);
        collectSorted(node->right.get(), ids);
    }

public:
    bool insert(int id) { return insert(root_, id); }
    bool contains(int id) const { return contains(root_.get(), id); }

    std::vector<int> sortedIds() const {
        std::vector<int> ids;
        collectSorted(root_.get(), ids);
        return ids;
    }
};

class HospitalMap {
private:
    const std::vector<std::string> locations_ = {
        "Main Lobby", "Emergency", "General Ward", "Pharmacy", "Laboratory", "Imaging"};
    const std::vector<std::vector<int>> connections_ = {
        {1, 2}, {0, 3, 4}, {0, 3, 5}, {1, 2}, {1, 5}, {2, 4}};

public:
    void printLocations() const {
        for (std::size_t i = 0; i < locations_.size(); ++i) {
            std::cout << i + 1 << ". " << locations_[i] << '\n';
        }
    }

    const std::vector<std::string>& locations() const { return locations_; }

    std::vector<std::string> route(const std::string& startName,
                                   const std::string& destinationName) const {
        const auto startLocation = std::find(locations_.begin(), locations_.end(), startName);
        const auto destinationLocation =
            std::find(locations_.begin(), locations_.end(), destinationName);
        if (startLocation == locations_.end() || destinationLocation == locations_.end()) {
            return {};
        }

        const int start = static_cast<int>(startLocation - locations_.begin());
        const int destination = static_cast<int>(destinationLocation - locations_.begin());
        const int count = static_cast<int>(locations_.size());

        std::queue<int> pending;
        std::vector<int> previous(count, -1);
        std::vector<bool> visited(count, false);
        pending.push(start);
        visited[start] = true;

        while (!pending.empty()) {
            const int current = pending.front();
            pending.pop();
            if (current == destination) {
                break;
            }
            for (int neighbor : connections_[current]) {
                if (!visited[neighbor]) {
                    visited[neighbor] = true;
                    previous[neighbor] = current;
                    pending.push(neighbor);
                }
            }
        }

        if (!visited[destination]) {
            return {};
        }

        std::vector<int> route;
        for (int place = destination; place != -1; place = previous[place]) {
            route.push_back(place);
        }
        std::reverse(route.begin(), route.end());

        std::vector<std::string> result;
        for (int place : route) {
            result.push_back(locations_[place]);
        }
        return result;
    }
};

class HospitalSystem {
private:
    std::list<Patient> patients_;
    std::queue<int> triageQueue_;
    PatientIdTree patientIds_;
    HospitalMap map_;
    int nextId_ = 1001;

    Patient* findPatient(int id) {
        if (!patientIds_.contains(id)) {
            return nullptr;
        }
        auto patient = std::find_if(patients_.begin(), patients_.end(),
                                    [id](const Patient& entry) { return entry.id == id; });
        return patient == patients_.end() ? nullptr : &*patient;
    }

    static std::string readLine(const std::string& prompt) {
        std::cout << prompt;
        std::string value;
        std::getline(std::cin, value);
        return value;
    }

    static int readInt(const std::string& prompt) {
        const std::string input = readLine(prompt);
        std::istringstream parser(input);
        int value;
        char extra;
        if (!(parser >> value) || (parser >> extra)) {
            return -1;
        }
        return value;
    }

    static void printPatient(const Patient& patient) {
        std::cout << "ID: " << patient.id << " | Name: " << patient.name
                  << " | Age: " << patient.age << " | Condition: " << patient.condition
                  << " | Status: " << patient.status << '\n';
    }

    void registerPatient() {
        const std::string name = readLine("Patient name: ");
        const int age = readInt("Age: ");
        const std::string condition = readLine("Reason for visit: ");
        if (name.empty() || condition.empty() || age < 0) {
            std::cout << "Please enter a name, a valid age, and a reason for visit.\n";
            return;
        }

        const Patient patient = addPatient(name, age, condition);
        std::cout << "Registered patient with ID " << patient.id
                  << " and added them to the triage queue.\n";
    }

    void listPatients() const {
        if (patients_.empty()) {
            std::cout << "No patients registered.\n";
            return;
        }
        for (const Patient& patient : patients_) {
            printPatient(patient);
        }
    }

    void searchPatient() {
        const int id = readInt("Patient ID: ");
        Patient* patient = findPatient(id);
        if (patient) {
            printPatient(*patient);
        } else {
            std::cout << "Patient not found.\n";
        }
    }

    Patient* admitNextPatient() {
        while (!triageQueue_.empty()) {
            const int id = triageQueue_.front();
            triageQueue_.pop();
            Patient* patient = findPatient(id);
            if (patient && patient->status == "Waiting for triage") {
                patient->status = "Admitted";
                return patient;
            }
        }
        return nullptr;
    }

    void dischargePatient() {
        const int id = readInt("Patient ID to discharge: ");
        Patient* patient = findPatient(id);
        if (!patient) {
            std::cout << "Patient not found.\n";
        } else if (patient->status != "Admitted") {
            std::cout << "Only admitted patients can be discharged.\n";
        } else if (discharge(id)) {
            std::cout << "Patient " << patient->id << " discharged.\n";
        }
    }

    void showRoute() const {
        std::cout << "Hospital locations:\n";
        map_.printLocations();
        const int start = readInt("Start location number: ");
        const int destination = readInt("Destination location number: ");
        const auto& locations = map_.locations();
        if (start < 1 || start > static_cast<int>(locations.size()) || destination < 1 ||
            destination > static_cast<int>(locations.size())) {
            std::cout << "Please choose valid location numbers.\n";
            return;
        }
        const std::vector<std::string> path = map_.route(locations[start - 1], locations[destination - 1]);
        if (path.empty()) {
            std::cout << "No route found.\n";
            return;
        }
        std::cout << "Route: ";
        for (std::size_t i = 0; i < path.size(); ++i) {
            if (i > 0) {
                std::cout << " -> ";
            }
            std::cout << path[i];
        }
        std::cout << '\n';
    }

public:
    void run() {
        while (true) {
            std::cout << "\n=== Hospital Patient Management ===\n"
                      << "1. Register patient\n"
                      << "2. List patients\n"
                      << "3. Search patient by ID\n"
                      << "4. Admit next patient\n"
                      << "5. Discharge patient\n"
                      << "6. Find route in hospital\n"
                      << "7. Show patient IDs (sorted tree traversal)\n"
                      << "0. Exit\n";
            const int choice = readInt("Choose an option: ");
            if (std::cin.eof()) {
                return;
            }
            switch (choice) {
                case 1: registerPatient(); break;
                case 2: listPatients(); break;
                case 3: searchPatient(); break;
                case 4: {
                    Patient* patient = admitNextPatient();
                    if (patient) {
                        std::cout << "Admitted next patient: ";
                        printPatient(*patient);
                    } else {
                        std::cout << "The triage queue is empty.\n";
                    }
                    break;
                }
                case 5: dischargePatient(); break;
                case 6: showRoute(); break;
                case 7:
                    std::cout << "Patient IDs in ascending order: ";
                    for (int id : patientIds_.sortedIds()) {
                        std::cout << id << ' ';
                    }
                    std::cout << '\n';
                    break;
                case 0: return;
                default: std::cout << "Invalid option. Choose 0-7.\n"; break;
            }
        }
    }

    std::vector<Patient> patients() const {
        return {patients_.begin(), patients_.end()};
    }

    Patient* getPatient(int id) { return findPatient(id); }

    Patient* admitNext() { return admitNextPatient(); }

    Patient addPatient(const std::string& name, int age, const std::string& condition) {
        if (name.empty() || condition.empty() || age < 0 || age > 130) {
            throw std::invalid_argument("Enter a name, a reason for visit, and an age from 0 to 130.");
        }
        Patient patient{nextId_++, name, age, condition, "Waiting for triage"};
        patientIds_.insert(patient.id);
        patients_.push_back(patient);
        triageQueue_.push(patient.id);
        return patient;
    }

    bool discharge(int id) {
        Patient* patient = findPatient(id);
        if (!patient || patient->status != "Admitted") {
            return false;
        }
        patient->status = "Discharged";
        return true;
    }

    std::size_t waitingCount() const {
        return static_cast<std::size_t>(std::count_if(
            patients_.begin(), patients_.end(), [](const Patient& patient) {
                return patient.status == "Waiting for triage";
            }));
    }

    std::vector<int> sortedPatientIds() const { return patientIds_.sortedIds(); }

    const std::vector<std::string>& locations() const { return map_.locations(); }

    std::vector<std::string> route(const std::string& from, const std::string& to) const {
        return map_.route(from, to);
    }
};

namespace {

json patientToJson(const Patient& patient) {
    return {{"id", patient.id},
            {"name", patient.name},
            {"age", patient.age},
            {"condition", patient.condition},
            {"status", patient.status}};
}

void sendJson(httplib::Response& response, int status, const json& body) {
    response.status = status;
    response.set_content(body.dump(), "application/json");
}

}  // namespace

int main(int argc, char* argv[]) {
    HospitalSystem hospital;
    if (argc > 1 && std::string(argv[1]) == "--console") {
        hospital.run();
        return 0;
    }

    httplib::Server server;
    server.set_mount_point("/", HOSPITAL_WEB_DIR);

    server.Get("/api/health", [](const httplib::Request&, httplib::Response& response) {
        sendJson(response, 200, {{"status", "ok"}});
    });

    server.Get("/api/patients", [&hospital](const httplib::Request&, httplib::Response& response) {
        json records = json::array();
        int admitted = 0;
        for (const Patient& patient : hospital.patients()) {
            records.push_back(patientToJson(patient));
            if (patient.status == "Admitted") {
                ++admitted;
            }
        }
        sendJson(response, 200,
                 {{"patients", records},
                  {"total", records.size()},
                  {"waiting", hospital.waitingCount()},
                  {"admitted", admitted}});
    });

    server.Post("/api/patients", [&hospital](const httplib::Request& request,
                                              httplib::Response& response) {
        try {
            const json body = json::parse(request.body);
            if (!body.contains("name") || !body["name"].is_string() ||
                !body.contains("age") || !body["age"].is_number_integer() ||
                !body.contains("condition") || !body["condition"].is_string()) {
                sendJson(response, 400,
                         {{"error", "Provide name, integer age, and condition fields."}});
                return;
            }
            const Patient patient = hospital.addPatient(body["name"].get<std::string>(),
                                                        body["age"].get<int>(),
                                                        body["condition"].get<std::string>());
            sendJson(response, 201, patientToJson(patient));
        } catch (const std::exception& error) {
            sendJson(response, 400, {{"error", error.what()}});
        }
    });

    server.Get(R"(/api/patients/([0-9]+))",
               [&hospital](const httplib::Request& request, httplib::Response& response) {
                   try {
                       Patient* patient = hospital.getPatient(std::stoi(request.matches[1]));
                       if (!patient) {
                           sendJson(response, 404, {{"error", "Patient not found."}});
                           return;
                       }
                       sendJson(response, 200, patientToJson(*patient));
                   } catch (const std::exception&) {
                       sendJson(response, 400, {{"error", "Invalid patient ID."}});
                   }
               });

    server.Post(R"(/api/patients/([0-9]+)/discharge)",
                [&hospital](const httplib::Request& request, httplib::Response& response) {
                    try {
                        const int id = std::stoi(request.matches[1]);
                        Patient* patient = hospital.getPatient(id);
                        if (!patient) {
                            sendJson(response, 404, {{"error", "Patient not found."}});
                        } else if (!hospital.discharge(id)) {
                            sendJson(response, 409,
                                     {{"error", "Only admitted patients can be discharged."}});
                        } else {
                            sendJson(response, 200, patientToJson(*patient));
                        }
                    } catch (const std::exception&) {
                        sendJson(response, 400, {{"error", "Invalid patient ID."}});
                    }
                });

    server.Post("/api/triage/next", [&hospital](const httplib::Request&,
                                                 httplib::Response& response) {
        Patient* patient = hospital.admitNext();
        if (!patient) {
            sendJson(response, 409, {{"error", "The triage queue is empty."}});
            return;
        }
        sendJson(response, 200, patientToJson(*patient));
    });

    server.Get("/api/patient-ids", [&hospital](const httplib::Request&,
                                                httplib::Response& response) {
        sendJson(response, 200, {{"ids", hospital.sortedPatientIds()}});
    });

    server.Get("/api/locations", [&hospital](const httplib::Request&,
                                              httplib::Response& response) {
        sendJson(response, 200, {{"locations", hospital.locations()}});
    });

    server.Get("/api/route", [&hospital](const httplib::Request& request,
                                          httplib::Response& response) {
        if (!request.has_param("from") || !request.has_param("to")) {
            sendJson(response, 400, {{"error", "Provide from and to locations."}});
            return;
        }
        const std::string from = request.get_param_value("from");
        const std::string to = request.get_param_value("to");
        const std::vector<std::string> path = hospital.route(from, to);
        if (path.empty()) {
            sendJson(response, 400, {{"error", "Choose valid hospital locations."}});
            return;
        }
        sendJson(response, 200, {{"route", path}});
    });

    std::cout << "Hospital dashboard: http://127.0.0.1:8080\n";
    std::cout << "Use Ctrl+C to stop, or pass --console for the text menu.\n";
    if (!server.listen("127.0.0.1", 8080)) {
        std::cerr << "Could not start server on 127.0.0.1:8080\n";
        return 1;
    }
    return 0;
}