#pragma once

#include <algorithm>
#include <cmath>
#include <fstream>
#include <numbers>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace evolab::io {

enum class EdgeWeightType {
    EUC_2D,  // Euclidean distances in 2D
    EUC_3D,  // Euclidean distances in 3D
    MAX_2D,  // Maximum distances in 2D
    MAX_3D,  // Maximum distances in 3D
    MAN_2D,  // Manhattan distances in 2D
    MAN_3D,  // Manhattan distances in 3D
    CEIL_2D, // Ceiling of Euclidean distances in 2D
    GEO,     // Geographical distances
    ATT,     // Special distance function (ATT)
    XRAY1,   // Special distance function (XRAY1)
    XRAY2,   // Special distance function (XRAY2)
    EXPLICIT // Explicit distance matrix
};

enum class EdgeWeightFormat {
    FUNCTION,       // Computed from coordinates
    FULL_MATRIX,    // Full n×n matrix
    UPPER_ROW,      // Upper triangular matrix row-wise
    LOWER_ROW,      // Lower triangular matrix row-wise
    UPPER_DIAG_ROW, // Upper triangular matrix with diagonal row-wise
    LOWER_DIAG_ROW, // Lower triangular matrix with diagonal row-wise
    UPPER_COL,      // Upper triangular matrix column-wise
    LOWER_COL,      // Lower triangular matrix column-wise
    UPPER_DIAG_COL, // Upper triangular matrix with diagonal column-wise
    LOWER_DIAG_COL  // Lower triangular matrix with diagonal column-wise
};

struct TSPInstance {
    std::string name;
    std::string comment;
    int dimension = 0;
    EdgeWeightType edge_weight_type = EdgeWeightType::EUC_2D;
    EdgeWeightFormat edge_weight_format = EdgeWeightFormat::FUNCTION;

    std::vector<std::pair<double, double>> node_coords;
    std::vector<std::pair<double, double>> display_coords;
    std::vector<double> distance_matrix;

    double calculate_distance(int i, int j) const;
    std::vector<double> get_full_distance_matrix() const;
};

class TSPLIBParser {
  public:
    static TSPInstance parse_file(const std::string& filename);
    static TSPInstance parse_string(const std::string& content);
    static TSPInstance parse_stream(std::istream& stream);

    static void write_tour_file(const std::string& filename, const std::string& problem_name,
                                const std::vector<int>& tour, double tour_length = -1.0);

    // Distance calculation functions (public for TSPInstance to use)
    static double euclidean_2d(double x1, double y1, double x2, double y2);
    static double euclidean_2d_raw(double x1, double y1, double x2, double y2);
    static double euclidean_3d(double x1, double y1, double z1, double x2, double y2, double z2);
    static double manhattan_2d(double x1, double y1, double x2, double y2);
    static double manhattan_3d(double x1, double y1, double z1, double x2, double y2, double z2);
    static double maximum_2d(double x1, double y1, double x2, double y2);
    static double maximum_3d(double x1, double y1, double z1, double x2, double y2, double z2);
    static double geographical(double lat1, double lon1, double lat2, double lon2);
    static double att_distance(double x1, double y1, double x2, double y2);

  private:
    static EdgeWeightType parse_edge_weight_type(const std::string& type_str);
    static EdgeWeightFormat parse_edge_weight_format(const std::string& format_str);

    static void parse_header(const std::string& line, TSPInstance& instance);
    static void parse_node_coord_section(std::istream& stream, TSPInstance& instance);
    static void parse_display_data_section(std::istream& stream, TSPInstance& instance);
    static void parse_edge_weight_section(std::istream& stream, TSPInstance& instance);

    // Helper function to check if line starts with keyword after trimming
    static bool line_starts_with(const std::string& line, const std::string& keyword);
};

// Implementation

inline double TSPInstance::calculate_distance(int i, int j) const {
    if (i == j)
        return 0.0;
    if (i < 0 || i >= dimension || j < 0 || j >= dimension) {
        throw std::out_of_range("Node indices out of range");
    }

    if (edge_weight_type == EdgeWeightType::EXPLICIT) {
        switch (edge_weight_format) {
        case EdgeWeightFormat::FULL_MATRIX:
            return distance_matrix[i * dimension + j];
        case EdgeWeightFormat::UPPER_ROW: {
            if (i > j)
                std::swap(i, j);
            // For upper triangular: row i has (dimension - i - 1) elements
            // Starting position for row i: sum of previous row lengths
            int offset = i * dimension - (i * (i + 1)) / 2;
            return distance_matrix[offset + (j - i - 1)];
        }
        case EdgeWeightFormat::LOWER_ROW: {
            if (i < j)
                std::swap(i, j);
            int offset = i * (i - 1) / 2;
            return distance_matrix[offset + j];
        }
        case EdgeWeightFormat::UPPER_DIAG_ROW: {
            if (i <= j) {
                // For upper triangular with diagonal: row i has (dimension - i) elements
                // Starting position for row i: sum of previous row lengths
                int offset = i * dimension - (i * (i - 1)) / 2;
                return distance_matrix[offset + (j - i)];
            } else {
                // Symmetric matrix
                int offset = j * dimension - (j * (j - 1)) / 2;
                return distance_matrix[offset + (i - j)];
            }
        }
        case EdgeWeightFormat::LOWER_DIAG_ROW: {
            if (i >= j) {
                int offset = i * (i + 1) / 2;
                return distance_matrix[offset + j];
            } else {
                int offset = j * (j + 1) / 2;
                return distance_matrix[offset + i];
            }
        }
        case EdgeWeightFormat::UPPER_COL: {
            // Upper triangular column-wise: column j has j elements (0 to j-1)
            if (i < j) {
                int offset = j * (j - 1) / 2;
                return distance_matrix[offset + i];
            } else {
                // Symmetric matrix
                int offset = i * (i - 1) / 2;
                return distance_matrix[offset + j];
            }
        }
        case EdgeWeightFormat::LOWER_COL: {
            // Lower triangular column-wise: column j has (dimension - j) elements
            if (i >= j) {
                int offset = j * dimension - (j * (j - 1)) / 2;
                return distance_matrix[offset + (i - j)];
            } else {
                // Symmetric matrix
                int offset = i * dimension - (i * (i - 1)) / 2;
                return distance_matrix[offset + (j - i)];
            }
        }
        case EdgeWeightFormat::UPPER_DIAG_COL: {
            // Upper triangular with diagonal column-wise: column j has j+1 elements (0 to j)
            if (i <= j) {
                int offset = j * (j + 1) / 2;
                return distance_matrix[offset + i];
            } else {
                // Symmetric matrix
                int offset = i * (i + 1) / 2;
                return distance_matrix[offset + j];
            }
        }
        case EdgeWeightFormat::LOWER_DIAG_COL: {
            // Lower triangular with diagonal column-wise: column j has (dimension - j) elements
            if (i >= j) {
                int offset = j * dimension - (j * (j - 1)) / 2;
                return distance_matrix[offset + (i - j)];
            } else {
                // Symmetric matrix
                int offset = i * dimension - (i * (i - 1)) / 2;
                return distance_matrix[offset + (j - i)];
            }
        }
        default:
            throw std::runtime_error("Unsupported edge weight format");
        }
    }

    if (node_coords.empty()) {
        throw std::runtime_error("No coordinate data available");
    }

    double x1 = node_coords[i].first, y1 = node_coords[i].second;
    double x2 = node_coords[j].first, y2 = node_coords[j].second;

    switch (edge_weight_type) {
    case EdgeWeightType::EUC_2D:
        return TSPLIBParser::euclidean_2d(x1, y1, x2, y2);
    case EdgeWeightType::EUC_3D:
        // For 3D, assume z=0 if only 2D coordinates are available
        return TSPLIBParser::euclidean_3d(x1, y1, 0.0, x2, y2, 0.0);
    case EdgeWeightType::CEIL_2D:
        return std::ceil(TSPLIBParser::euclidean_2d_raw(x1, y1, x2, y2));
    case EdgeWeightType::MAN_2D:
        return TSPLIBParser::manhattan_2d(x1, y1, x2, y2);
    case EdgeWeightType::MAN_3D:
        // For 3D, assume z=0 if only 2D coordinates are available
        return TSPLIBParser::manhattan_3d(x1, y1, 0.0, x2, y2, 0.0);
    case EdgeWeightType::MAX_2D:
        return TSPLIBParser::maximum_2d(x1, y1, x2, y2);
    case EdgeWeightType::MAX_3D:
        // For 3D, assume z=0 if only 2D coordinates are available
        return TSPLIBParser::maximum_3d(x1, y1, 0.0, x2, y2, 0.0);
    case EdgeWeightType::GEO:
        return TSPLIBParser::geographical(x1, y1, x2, y2);
    case EdgeWeightType::ATT:
        return TSPLIBParser::att_distance(x1, y1, x2, y2);
    case EdgeWeightType::XRAY1:
    case EdgeWeightType::XRAY2:
        throw std::runtime_error("XRAY1 and XRAY2 distance types are not implemented");
    default:
        throw std::runtime_error("Unsupported edge weight type");
    }
}

inline std::vector<double> TSPInstance::get_full_distance_matrix() const {
    std::vector<double> matrix(dimension * dimension);

    for (int i = 0; i < dimension; ++i) {
        for (int j = 0; j < dimension; ++j) {
            matrix[i * dimension + j] = calculate_distance(i, j);
        }
    }

    return matrix;
}

inline TSPInstance TSPLIBParser::parse_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    return parse_stream(file);
}

inline TSPInstance TSPLIBParser::parse_string(const std::string& content) {
    std::istringstream stream(content);
    return parse_stream(stream);
}

inline TSPInstance TSPLIBParser::parse_stream(std::istream& stream) {
    TSPInstance instance;
    std::string line;

    // Parse header
    while (std::getline(stream, line)) {
        if (line_starts_with(line, "NODE_COORD_SECTION") ||
            line_starts_with(line, "DISPLAY_DATA_SECTION") ||
            line_starts_with(line, "EDGE_WEIGHT_SECTION")) {
            break;
        }
        if (!line.empty()) {
            parse_header(line, instance);
        }
    }

    if (instance.dimension <= 0) {
        throw std::runtime_error("Invalid dimension specification");
    }

    // Parse sections
    do {
        if (line_starts_with(line, "NODE_COORD_SECTION")) {
            parse_node_coord_section(stream, instance);
        } else if (line_starts_with(line, "DISPLAY_DATA_SECTION")) {
            parse_display_data_section(stream, instance);
        } else if (line_starts_with(line, "EDGE_WEIGHT_SECTION")) {
            parse_edge_weight_section(stream, instance);
        }
    } while (std::getline(stream, line) && line != "EOF");

    return instance;
}

inline void TSPLIBParser::write_tour_file(const std::string& filename,
                                          const std::string& problem_name,
                                          const std::vector<int>& tour, double tour_length) {
    std::ofstream file(filename);
    if (!file) {
        throw std::runtime_error("Cannot create tour file: " + filename);
    }

    file << "NAME : " << problem_name << "\n";
    file << "COMMENT : Tour generated by EvoLab\n";
    file << "TYPE : TOUR\n";
    file << "DIMENSION : " << tour.size() << "\n";
    if (tour_length >= 0) {
        file << "LENGTH : " << tour_length << "\n";
    }
    file << "TOUR_SECTION\n";

    for (int city : tour) {
        file << (city + 1) << "\n"; // TSPLIB uses 1-based indexing
    }

    file << "-1\nEOF\n";
}

inline EdgeWeightType TSPLIBParser::parse_edge_weight_type(const std::string& type_str) {
    static const std::unordered_map<std::string, EdgeWeightType> type_map = {
        {"EUC_2D", EdgeWeightType::EUC_2D},   {"EUC_3D", EdgeWeightType::EUC_3D},
        {"MAX_2D", EdgeWeightType::MAX_2D},   {"MAX_3D", EdgeWeightType::MAX_3D},
        {"MAN_2D", EdgeWeightType::MAN_2D},   {"MAN_3D", EdgeWeightType::MAN_3D},
        {"CEIL_2D", EdgeWeightType::CEIL_2D}, {"GEO", EdgeWeightType::GEO},
        {"ATT", EdgeWeightType::ATT},         {"XRAY1", EdgeWeightType::XRAY1},
        {"XRAY2", EdgeWeightType::XRAY2},     {"EXPLICIT", EdgeWeightType::EXPLICIT}};

    auto it = type_map.find(type_str);
    if (it == type_map.end()) {
        throw std::runtime_error("Unsupported edge weight type: " + type_str);
    }
    return it->second;
}

inline EdgeWeightFormat TSPLIBParser::parse_edge_weight_format(const std::string& format_str) {
    static const std::unordered_map<std::string, EdgeWeightFormat> format_map = {
        {"FUNCTION", EdgeWeightFormat::FUNCTION},
        {"FULL_MATRIX", EdgeWeightFormat::FULL_MATRIX},
        {"UPPER_ROW", EdgeWeightFormat::UPPER_ROW},
        {"LOWER_ROW", EdgeWeightFormat::LOWER_ROW},
        {"UPPER_DIAG_ROW", EdgeWeightFormat::UPPER_DIAG_ROW},
        {"LOWER_DIAG_ROW", EdgeWeightFormat::LOWER_DIAG_ROW},
        {"UPPER_COL", EdgeWeightFormat::UPPER_COL},
        {"LOWER_COL", EdgeWeightFormat::LOWER_COL},
        {"UPPER_DIAG_COL", EdgeWeightFormat::UPPER_DIAG_COL},
        {"LOWER_DIAG_COL", EdgeWeightFormat::LOWER_DIAG_COL}};

    auto it = format_map.find(format_str);
    if (it == format_map.end()) {
        throw std::runtime_error("Unsupported edge weight format: " + format_str);
    }
    return it->second;
}

inline void TSPLIBParser::parse_header(const std::string& line, TSPInstance& instance) {
    size_t colon_pos = line.find(':');
    if (colon_pos == std::string::npos)
        return;

    std::string key = line.substr(0, colon_pos);
    std::string value = line.substr(colon_pos + 1);

    // Trim whitespace from key and value
    key.erase(0, key.find_first_not_of(" \t"));
    key.erase(key.find_last_not_of(" \t") + 1);
    value.erase(0, value.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t\r\n") + 1);

    if (key == "NAME") {
        instance.name = value;
    } else if (key == "COMMENT") {
        instance.comment = value;
    } else if (key == "DIMENSION") {
        instance.dimension = std::stoi(value);
    } else if (key == "EDGE_WEIGHT_TYPE") {
        instance.edge_weight_type = parse_edge_weight_type(value);
    } else if (key == "EDGE_WEIGHT_FORMAT") {
        instance.edge_weight_format = parse_edge_weight_format(value);
    }
}

inline void TSPLIBParser::parse_node_coord_section(std::istream& stream, TSPInstance& instance) {
    instance.node_coords.resize(instance.dimension);

    std::string line;
    for (int i = 0; i < instance.dimension && std::getline(stream, line); ++i) {
        if (line.empty() || line == "EOF")
            break;

        std::istringstream iss(line);
        int node_id;
        double x, y;

        if (!(iss >> node_id >> x >> y)) {
            throw std::runtime_error("Invalid node coordinate format at line: " + line);
        }

        // Validate node_id and use it for proper indexing
        if (node_id < 1 || node_id > instance.dimension) {
            throw std::runtime_error("Invalid node ID: " + std::to_string(node_id));
        }

        instance.node_coords[node_id - 1] = {x, y};
    }
}

inline void TSPLIBParser::parse_display_data_section(std::istream& stream, TSPInstance& instance) {
    instance.display_coords.resize(instance.dimension);

    std::string line;
    for (int i = 0; i < instance.dimension && std::getline(stream, line); ++i) {
        if (line.empty() || line == "EOF")
            break;

        std::istringstream iss(line);
        int node_id;
        double x, y;

        if (!(iss >> node_id >> x >> y)) {
            throw std::runtime_error("Invalid display coordinate format at line: " + line);
        }

        // Validate node_id and use it for proper indexing
        if (node_id < 1 || node_id > instance.dimension) {
            throw std::runtime_error("Invalid display node ID: " + std::to_string(node_id));
        }

        instance.display_coords[node_id - 1] = {x, y};
    }
}

inline void TSPLIBParser::parse_edge_weight_section(std::istream& stream, TSPInstance& instance) {
    size_t expected_size = 0;

    switch (instance.edge_weight_format) {
    case EdgeWeightFormat::FULL_MATRIX:
        expected_size = instance.dimension * instance.dimension;
        break;
    case EdgeWeightFormat::UPPER_ROW:
    case EdgeWeightFormat::LOWER_ROW:
        expected_size = (instance.dimension * (instance.dimension - 1)) / 2;
        break;
    case EdgeWeightFormat::UPPER_DIAG_ROW:
    case EdgeWeightFormat::LOWER_DIAG_ROW:
    case EdgeWeightFormat::UPPER_DIAG_COL:
    case EdgeWeightFormat::LOWER_DIAG_COL:
        expected_size = (instance.dimension * (instance.dimension + 1)) / 2;
        break;
    case EdgeWeightFormat::UPPER_COL:
    case EdgeWeightFormat::LOWER_COL:
        expected_size = (instance.dimension * (instance.dimension - 1)) / 2;
        break;
    default:
        throw std::runtime_error("Unsupported edge weight format for parsing");
    }

    instance.distance_matrix.reserve(expected_size);

    std::string line;
    while (std::getline(stream, line) && instance.distance_matrix.size() < expected_size) {
        if (line.empty() || line == "EOF")
            break;

        std::istringstream iss(line);
        double distance;

        while (iss >> distance && instance.distance_matrix.size() < expected_size) {
            instance.distance_matrix.push_back(distance);
        }
    }

    if (instance.distance_matrix.size() != expected_size) {
        throw std::runtime_error("Distance matrix size mismatch");
    }
}

inline double TSPLIBParser::euclidean_2d(double x1, double y1, double x2, double y2) {
    double dx = x1 - x2;
    double dy = y1 - y2;
    return std::round(std::sqrt(dx * dx + dy * dy));
}

inline double TSPLIBParser::euclidean_2d_raw(double x1, double y1, double x2, double y2) {
    double dx = x1 - x2;
    double dy = y1 - y2;
    return std::sqrt(dx * dx + dy * dy);
}

inline double TSPLIBParser::euclidean_3d(double x1, double y1, double z1, double x2, double y2,
                                         double z2) {
    double dx = x1 - x2;
    double dy = y1 - y2;
    double dz = z1 - z2;
    return std::round(std::sqrt(dx * dx + dy * dy + dz * dz));
}

inline double TSPLIBParser::manhattan_2d(double x1, double y1, double x2, double y2) {
    return std::abs(x1 - x2) + std::abs(y1 - y2);
}

inline double TSPLIBParser::manhattan_3d(double x1, double y1, double z1, double x2, double y2,
                                         double z2) {
    return std::abs(x1 - x2) + std::abs(y1 - y2) + std::abs(z1 - z2);
}

inline double TSPLIBParser::maximum_2d(double x1, double y1, double x2, double y2) {
    return std::max(std::abs(x1 - x2), std::abs(y1 - y2));
}

inline double TSPLIBParser::maximum_3d(double x1, double y1, double z1, double x2, double y2,
                                       double z2) {
    return std::max({std::abs(x1 - x2), std::abs(y1 - y2), std::abs(z1 - z2)});
}

inline double TSPLIBParser::geographical(double lat1, double lon1, double lat2, double lon2) {
    constexpr double RRR = 6378.388;
    constexpr double PI = std::numbers::pi;

    auto deg_to_rad = [](double deg) {
        int int_deg = static_cast<int>(deg);
        double min_part = deg - int_deg;
        return PI * (int_deg + 5.0 * min_part / 3.0) / 180.0;
    };

    double q1 = std::cos(deg_to_rad(lon1) - deg_to_rad(lon2));
    double q2 = std::cos(deg_to_rad(lat1) - deg_to_rad(lat2));
    double q3 = std::cos(deg_to_rad(lat1) + deg_to_rad(lat2));

    return std::ceil(RRR * std::acos(0.5 * ((1.0 + q1) * q2 - (1.0 - q1) * q3)));
}

inline double TSPLIBParser::att_distance(double x1, double y1, double x2, double y2) {
    double dx = x1 - x2;
    double dy = y1 - y2;
    double rij = std::sqrt((dx * dx + dy * dy) / 10.0);
    return std::round(rij);
}

inline bool TSPLIBParser::line_starts_with(const std::string& line, const std::string& keyword) {
    // Trim leading whitespace from line
    size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos) {
        return false; // Empty line after trimming
    }

    // Check if the trimmed line starts with the keyword
    return line.substr(start, keyword.length()) == keyword;
}

} // namespace evolab::io