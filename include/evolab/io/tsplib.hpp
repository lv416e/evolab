#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
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

enum class TSPType {
    TSP,  // Symmetric Traveling Salesman Problem
    ATSP, // Asymmetric Traveling Salesman Problem
    HCP,  // Hamiltonian Cycle Problem
    SOP   // Sequential Ordering Problem
};

// Distance calculation utilities - separated from parser for better modularity
namespace tsp_distance {

inline double euclidean_2d(double x1, double y1, double x2, double y2) {
    double dx = x1 - x2;
    double dy = y1 - y2;
    return std::round(std::sqrt(dx * dx + dy * dy));
}

inline double euclidean_2d_raw(double x1, double y1, double x2, double y2) {
    double dx = x1 - x2;
    double dy = y1 - y2;
    return std::sqrt(dx * dx + dy * dy);
}

inline double euclidean_3d(double x1, double y1, double z1, double x2, double y2, double z2) {
    double dx = x1 - x2;
    double dy = y1 - y2;
    double dz = z1 - z2;
    return std::round(std::sqrt(dx * dx + dy * dy + dz * dz));
}

inline double manhattan_2d(double x1, double y1, double x2, double y2) {
    return std::abs(x1 - x2) + std::abs(y1 - y2);
}

inline double manhattan_3d(double x1, double y1, double z1, double x2, double y2, double z2) {
    return std::abs(x1 - x2) + std::abs(y1 - y2) + std::abs(z1 - z2);
}

inline double maximum_2d(double x1, double y1, double x2, double y2) {
    return std::max(std::abs(x1 - x2), std::abs(y1 - y2));
}

inline double maximum_3d(double x1, double y1, double z1, double x2, double y2, double z2) {
    return std::max({std::abs(x1 - x2), std::abs(y1 - y2), std::abs(z1 - z2)});
}

inline double geographical(double lat1, double lon1, double lat2, double lon2) {
    constexpr double RRR = 6378.388;
    constexpr double PI = std::numbers::pi;

    static auto deg_to_rad = [](double deg) {
        int int_deg = static_cast<int>(deg);
        double min_part = deg - int_deg;
        return PI * (int_deg + 5.0 * min_part / 3.0) / 180.0;
    };

    double q1 = std::cos(deg_to_rad(lon1) - deg_to_rad(lon2));
    double q2 = std::cos(deg_to_rad(lat1) - deg_to_rad(lat2));
    double q3 = std::cos(deg_to_rad(lat1) + deg_to_rad(lat2));

    return std::floor(RRR * std::acos(0.5 * ((1.0 + q1) * q2 - (1.0 - q1) * q3)) + 0.5);
}

inline double att_distance(double x1, double y1, double x2, double y2) {
    double dx = x1 - x2;
    double dy = y1 - y2;
    double rij = std::sqrt((dx * dx + dy * dy) / 10.0);
    return std::round(rij);
}

} // namespace tsp_distance

struct TSPInstance {
    std::string name;
    std::string comment;
    TSPType type = TSPType::TSP; // Default to symmetric TSP
    int dimension = 0;
    EdgeWeightType edge_weight_type = EdgeWeightType::EUC_2D;
    EdgeWeightFormat edge_weight_format = EdgeWeightFormat::FUNCTION;

    std::vector<std::array<double, 3>> node_coords;
    std::vector<std::array<double, 3>> display_coords;
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

  private:
    static EdgeWeightType parse_edge_weight_type(const std::string& type_str);
    static EdgeWeightFormat parse_edge_weight_format(const std::string& format_str);
    static TSPType parse_tsp_type(const std::string& type_str);

    static void parse_header(const std::string& line, TSPInstance& instance);
    static void parse_node_coord_section(std::istream& stream, TSPInstance& instance);
    static void parse_display_data_section(std::istream& stream, TSPInstance& instance);
    static void parse_edge_weight_section(std::istream& stream, TSPInstance& instance);

    // Helper function to check if line starts with keyword after trimming
    static bool line_starts_with(const std::string& line, const std::string& keyword);

    // Helper function to parse coordinate sections
    static void parse_coord_section(std::istream& stream, TSPInstance& instance,
                                    std::vector<std::array<double, 3>>& coords,
                                    const std::string& section_name);
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
            size_t offset =
                static_cast<size_t>(i) * dimension - (static_cast<size_t>(i) * (i + 1)) / 2;
            return distance_matrix[offset + (j - i - 1)];
        }
        case EdgeWeightFormat::LOWER_ROW: {
            if (i < j)
                std::swap(i, j);
            size_t offset = static_cast<size_t>(i) * (i - 1) / 2;
            return distance_matrix[offset + j];
        }
        case EdgeWeightFormat::UPPER_COL: {
            // Upper triangular column-wise: column j stores elements for rows 0 to j-1
            if (i > j)
                std::swap(i, j); // Now i <= j

            if (i == j)
                return 0.0; // Diagonal is 0

            // Column j has j elements (rows 0 to j-1)
            // Offset for start of column j = j * (j - 1) / 2
            size_t offset = static_cast<size_t>(j) * (j - 1) / 2;
            return distance_matrix[offset + i];
        }
        case EdgeWeightFormat::UPPER_DIAG_ROW:
        case EdgeWeightFormat::LOWER_DIAG_COL: {
            if (i <= j) {
                // For upper triangular with diagonal: row i has (dimension - i) elements
                // Starting position for row i: sum of previous row lengths
                size_t offset =
                    static_cast<size_t>(i) * dimension - (static_cast<size_t>(i) * (i - 1)) / 2;
                return distance_matrix[offset + (j - i)];
            } else {
                // Symmetric matrix
                size_t offset =
                    static_cast<size_t>(j) * dimension - (static_cast<size_t>(j) * (j - 1)) / 2;
                return distance_matrix[offset + (i - j)];
            }
        }
        case EdgeWeightFormat::LOWER_DIAG_ROW:
        case EdgeWeightFormat::UPPER_DIAG_COL: {
            if (i >= j) {
                size_t offset = static_cast<size_t>(i) * (i + 1) / 2;
                return distance_matrix[offset + j];
            } else {
                size_t offset = static_cast<size_t>(j) * (j + 1) / 2;
                return distance_matrix[offset + i];
            }
        }
        case EdgeWeightFormat::LOWER_COL: {
            // Lower triangular column-wise: column j stores elements for rows j+1 to dimension-1
            if (i < j)
                std::swap(i, j); // Now i >= j

            // For column j, we store elements from row j+1 to dimension-1
            // So we need i > j for non-diagonal elements
            if (i == j)
                return 0.0; // Diagonal is 0

            // Column j has (dimension - j - 1) elements
            // Offset for start of column j: mathematical formula for sum of arithmetic sequence
            size_t offset =
                static_cast<size_t>(j) * dimension - (static_cast<size_t>(j) * (j + 1)) / 2;
            return distance_matrix[offset + (i - j - 1)];
        }
        default:
            throw std::runtime_error("Unsupported edge weight format");
        }
    }

    if (node_coords.empty()) {
        throw std::runtime_error("No coordinate data available");
    }

    double x1 = node_coords[i][0], y1 = node_coords[i][1], z1 = node_coords[i][2];
    double x2 = node_coords[j][0], y2 = node_coords[j][1], z2 = node_coords[j][2];

    switch (edge_weight_type) {
    case EdgeWeightType::EUC_2D:
        return tsp_distance::euclidean_2d(x1, y1, x2, y2);
    case EdgeWeightType::EUC_3D:
        return tsp_distance::euclidean_3d(x1, y1, z1, x2, y2, z2);
    case EdgeWeightType::CEIL_2D:
        return std::ceil(tsp_distance::euclidean_2d_raw(x1, y1, x2, y2));
    case EdgeWeightType::MAN_2D:
        return tsp_distance::manhattan_2d(x1, y1, x2, y2);
    case EdgeWeightType::MAN_3D:
        return tsp_distance::manhattan_3d(x1, y1, z1, x2, y2, z2);
    case EdgeWeightType::MAX_2D:
        return tsp_distance::maximum_2d(x1, y1, x2, y2);
    case EdgeWeightType::MAX_3D:
        return tsp_distance::maximum_3d(x1, y1, z1, x2, y2, z2);
    case EdgeWeightType::GEO:
        return tsp_distance::geographical(x1, y1, x2, y2);
    case EdgeWeightType::ATT:
        return tsp_distance::att_distance(x1, y1, x2, y2);
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
    } while (std::getline(stream, line) && !line_starts_with(line, "EOF"));

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

inline TSPType TSPLIBParser::parse_tsp_type(const std::string& type_str) {
    static const std::unordered_map<std::string, TSPType> type_map = {{"TSP", TSPType::TSP},
                                                                      {"ATSP", TSPType::ATSP},
                                                                      {"HCP", TSPType::HCP},
                                                                      {"SOP", TSPType::SOP}};

    auto it = type_map.find(type_str);
    if (it == type_map.end()) {
        throw std::runtime_error("Unsupported TSP type: " + type_str);
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
        int parsed_dimension;
        auto [ptr, ec] =
            std::from_chars(value.data(), value.data() + value.size(), parsed_dimension);
        if (ec == std::errc()) {
            instance.dimension = parsed_dimension;
        } else {
            throw std::runtime_error("Invalid format for DIMENSION: " + value);
        }
    } else if (key == "EDGE_WEIGHT_TYPE") {
        instance.edge_weight_type = parse_edge_weight_type(value);
    } else if (key == "EDGE_WEIGHT_FORMAT") {
        instance.edge_weight_format = parse_edge_weight_format(value);
    } else if (key == "TYPE") {
        instance.type = parse_tsp_type(value);
    }
}

inline void TSPLIBParser::parse_node_coord_section(std::istream& stream, TSPInstance& instance) {
    parse_coord_section(stream, instance, instance.node_coords, "node");
}

inline void TSPLIBParser::parse_display_data_section(std::istream& stream, TSPInstance& instance) {
    parse_coord_section(stream, instance, instance.display_coords, "display");
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
        if (line.empty() || line_starts_with(line, "EOF"))
            break;

        // Parse doubles using high-performance char buffer parsing
        const char* p = line.data();
        const char* const end = p + line.size();
        char* endptr;

        while (p < end && instance.distance_matrix.size() < expected_size) {
            double distance = std::strtod(p, &endptr);
            if (p == endptr)
                break; // No number found or error
            instance.distance_matrix.push_back(distance);
            p = endptr;
        }
    }

    if (instance.distance_matrix.size() != expected_size) {
        throw std::runtime_error("Distance matrix size mismatch");
    }
}

inline void TSPLIBParser::parse_coord_section(std::istream& stream, TSPInstance& instance,
                                              std::vector<std::array<double, 3>>& coords,
                                              const std::string& section_name) {
    coords.resize(instance.dimension);

    std::string line;
    for (int i = 0; i < instance.dimension && std::getline(stream, line); ++i) {
        if (line.empty() || line_starts_with(line, "EOF"))
            break;

        // Parse coordinates using mixed approach - std::from_chars for int, manual for doubles
        const char* start = line.data();
        const char* end = line.data() + line.size();

        // Skip whitespace
        while (start < end && std::isspace(*start))
            ++start;

        int node_id;
        auto [ptr1, ec1] = std::from_chars(start, end, node_id);
        if (ec1 != std::errc()) {
            throw std::runtime_error("Invalid " + section_name +
                                     " node ID format at line: " + line);
        }

        double x, y, z = 0.0;

        // Parse coordinates directly from char buffer
        // Note: std::strtod automatically skips leading whitespace
        const char* p = ptr1;

        // Parse first coordinate (x)
        char* endptr;
        x = std::strtod(p, &endptr);
        if (endptr == p) {
            throw std::runtime_error("Invalid " + section_name +
                                     " coordinate format at line: " + line);
        }
        p = endptr;

        // Parse second coordinate (y) - std::strtod skips whitespace automatically
        y = std::strtod(p, &endptr);
        if (endptr == p) {
            throw std::runtime_error("Invalid " + section_name +
                                     " coordinate format at line: " + line);
        }
        p = endptr;

        // Try to parse optional z coordinate - std::strtod skips whitespace automatically
        if (p < end) {
            z = std::strtod(p, &endptr);
            // If parsing failed, z remains 0.0 (which is fine for optional coordinate)
        }

        // Validate node_id and use it for proper indexing
        if (node_id < 1 || node_id > instance.dimension) {
            throw std::runtime_error("Invalid " + section_name +
                                     " node ID: " + std::to_string(node_id));
        }

        coords[node_id - 1] = {x, y, z};
    }
}

inline bool TSPLIBParser::line_starts_with(const std::string& line, const std::string& keyword) {
    // Trim leading whitespace from line
    size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos) {
        return false; // Empty line after trimming
    }

    // Check if the trimmed line starts with the keyword (avoid substr allocation)
    return line.compare(start, keyword.length(), keyword) == 0;
}

} // namespace evolab::io
