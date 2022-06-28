#include <stdexcept>
#include <string>
#include <vector>
#include <string_view>
#include <algorithm>
#include "json_reader.h"
#include "json_builder.h"
#include "svg.h"
#include "transport_router.h"

using namespace std::literals::string_literals;

JsonReader::JsonReader(std::istream &input) {
    json_requests_ = std::move(std::make_unique<json::Node>(json::Load(input).GetRoot()));
}

void AddingBuses(transport_catalogue::TransportCatalogue& transport_catalogue_,
                 std::vector<const json::Node*>& buses_to_push) {
    for (const auto* bus_to_push : buses_to_push) {
        auto& bus_ = bus_to_push->AsDict();
        std::vector<std::string_view> stop_names;
        for (auto& stop : bus_.at("stops").AsArray()) {
            stop_names.push_back(stop.AsString());
        }
        std::vector<const transport_catalogue::Stop*> stops;
        stops.reserve(stop_names.size());

        for (auto& stop_name : stop_names) {
            auto temp_stop = transport_catalogue_.GetStop(stop_name);
            if (temp_stop) {
                stops.push_back(temp_stop);
            } else {
                throw std::invalid_argument("unknown stop_name: " + static_cast<std::string>(stop_name));
            }
        }

        bool round_flag = bus_to_push->AsDict().at("is_roundtrip").AsBool();
        std::string bus_name = bus_.at("name").AsString();
        transport_catalogue_.AddBus({bus_name, stop_names, round_flag});
    }
}

void JsonReader::AddToCatalogue(transport_catalogue::TransportCatalogue &transport_catalogue_) {
    auto& base_requests_array = json_requests_->AsDict().at("base_requests").AsArray();
    std::vector<const json::Node*> buses_to_push;
    std::map<std::string, json::Dict> distances_to_push;

    for (const auto& base_request : base_requests_array) {
        auto& request = base_request.AsDict();
        if (request.at("type").AsString() == "Stop") {
            std::string stop_name = request.at("name").AsString();
            geo::Coordinates coordinates {};
            coordinates.lat = request.at("latitude").AsDouble();
            coordinates.lng = request.at("longitude").AsDouble();
            transport_catalogue_.AddStop(stop_name, coordinates.lat, coordinates.lng);
            distances_to_push[stop_name] = request.at("road_distances").AsDict();
        } else {
            buses_to_push.push_back(&base_request);
        }
    }
    for (std::pair<const std::string, json::Dict>& distance : distances_to_push) {
        for (std::pair<std::string, json::Node> distanceTO : distance.second) {
            transport_catalogue_.SetDistanceBetweenStops(distance.first,
                                                         distanceTO.first, distanceTO.second.AsInt());
        }
    }
    AddingBuses(transport_catalogue_, buses_to_push);
}

svg::Color MakeColor(const json::Node& node_color) {
    if (node_color.IsString()) {
        return svg::Color{node_color.AsString()};
    } else if (node_color.IsArray()) {
        const json::Array& color_array = node_color.AsArray();
        if (color_array.size() > 4 || color_array.size() < 3)
            throw std::logic_error("");
        uint8_t r = color_array[0].AsInt();
        uint8_t g = color_array[1].AsInt();
        uint8_t b = color_array[2].AsInt();
        if (color_array.size() == 3) {
            return svg::Color{svg::Rgb{r,g,b}};
        } else {
            return svg::Color{svg::Rgba{r, g, b, color_array[3].AsDouble()}};
        }
    }
    throw std::invalid_argument("");
}

maper::RenderOptions JsonReader::AddRenderOptions() {
    auto& render_settings_map = json_requests_->AsDict().at("render_settings").AsDict();
    maper::RenderOptions render_options;
    render_options.width = render_settings_map.at("width").AsDouble();
    render_options.height = render_settings_map.at("height").AsDouble();
    render_options.padding = render_settings_map.at("padding").AsDouble();
    render_options.line_width = render_settings_map.at("line_width").AsDouble();
    render_options.stop_radius = render_settings_map.at("stop_radius").AsDouble();
    render_options.bus_label_font_size = render_settings_map.at("bus_label_font_size").AsInt();
    render_options.bus_label_offset.x = render_settings_map.at("bus_label_offset").AsArray()[0].AsDouble();
    render_options.bus_label_offset.y = render_settings_map.at("bus_label_offset").AsArray()[1].AsDouble();
    render_options.stop_label_offset.x = render_settings_map.at("stop_label_offset").AsArray()[0].AsDouble();
    render_options.stop_label_offset.y = render_settings_map.at("stop_label_offset").AsArray()[1].AsDouble();
    render_options.stop_label_font_size = render_settings_map.at("stop_label_font_size").AsInt();
    render_options.underlayer_width = render_settings_map.at("underlayer_width").AsDouble();
    render_options.underlayer_color = MakeColor(render_settings_map.at("underlayer_color"));
    for (const json::Node& current_color : render_settings_map.at("color_palette").AsArray()) {
        render_options.color_palette.push_back(MakeColor(current_color));
    }
    return render_options;
}

transport_router::RouterSettings JsonReader::AddRouterSettings(const json::Dict& router_settings) const {
    transport_router::RouterSettings settings;
    settings.bus_wait_time_ = router_settings.at("bus_wait_time").AsInt();
    settings.bus_velocity_ = static_cast<double>(router_settings.at("bus_velocity").AsInt()) * (1000. / 60.);
    return settings;
}

void StopRequest(json::Array& json_requests_result,transport_catalogue::TransportCatalogue& transport_catalogue_, const json::Dict stat_request_map, int request_id) {
    const std::string& stop_name = stat_request_map.at("name").AsString();
    json::Array buses_array;
    if (transport_catalogue_.GetStop(stop_name)) {
        auto buses = transport_catalogue_.GetBusesByStop(stop_name);
        if (!buses.empty()) {
            for (const transport_catalogue::Bus* bus_name: buses) {
                buses_array.push_back(json::Node{static_cast<std::string>(bus_name->number)});
            }
            std::sort(buses_array.begin(), buses_array.end(),
                      [](const json::Node &a, const json::Node &b) { return a.AsString() < b.AsString(); });
        }
        json_requests_result.push_back(json::Builder{}.StartDict().Key("buses"s).Value(json::Node(buses_array)).Key("request_id"s).Value(request_id).EndDict().Build());
    } else {
        json_requests_result.push_back(json::Builder{}.StartDict().Key("error_message"s).Value(json::Node("not found"s))
                                               .Key("request_id"s).Value(request_id).EndDict().Build());
    }
}

void BusRequest(json::Array& json_requests_result,transport_catalogue::TransportCatalogue& transport_catalogue_, const json::Dict stat_request_map, int request_id) {
    if (!transport_catalogue_.GetBus(stat_request_map.at("name").AsString())) {
        json_requests_result.push_back(json::Builder{}.StartDict().Key("error_message"s).Value(json::Node("not found"s))
                                               .Key("request_id"s).Value(request_id).EndDict().Build());
    } else {
        const std::string &bus_name = stat_request_map.at("name").AsString();
        auto bus = transport_catalogue_.GetBus(bus_name);
        json_requests_result.push_back(json::Builder{}.StartDict().Key("curvature"s).Value(json::Node(bus->curvature))
                                               .Key("stop_count"s).Value(json::Node(static_cast<int>(bus->all_stops_count)))
                                               .Key("unique_stop_count"s).Value(json::Node(static_cast<int>(bus->unique_stops_count)))
                                               .Key("route_length"s).Value(json::Node(bus->route_length))
                                               .Key("request_id"s).Value(request_id).EndDict().Build());
    }
}

void MapRequest(std::optional<maper::Maper>& maper_, json::Array& json_requests_result,
                transport_catalogue::TransportCatalogue& transport_catalogue_, int request_id) {
    svg::Document svg = maper_->MakeMap(transport_catalogue_);
    std::ostringstream out;
    svg.Render(out);
    auto map_result = (json::Builder{}.StartDict().Key("map"s).Value(json::Node(out.str())).Key("request_id"s).Value(request_id).EndDict().Build());
    json_requests_result.push_back(map_result);
}

void RouteRequest(json::Array& json_requests_result,transport_router::TransportRouter& transport_router_, 
                  const json::Dict stat_request_map, int request_id) {
    std::string from = stat_request_map.at("from").AsString();
    std::string to = stat_request_map.at("to").AsString();

    std::optional<std::vector<transport_router::RouterInfo>> edges_infos = transport_router_.FindFastestRoute(from, to);
    if (!edges_infos) {
        json_requests_result.push_back(json::Builder{}.StartDict().Key("request_id"s).Value(request_id).Key("error_message"s)
                                               .Value("not found"s).EndDict().Build());
    } else {
        double total_time = 0;
        int wait_time = transport_router_.GetRouterSettings().bus_wait_time_;
        json::Array items;
        for (transport_router::RouterInfo edge: edges_infos.value()) {
            json::Dict wait_item = json::Builder{}.StartDict().Key("type"s).Value("Wait"s).Key(
                    "stop_name"s).Value(std::string(edge.stop_from)).Key("time"s).Value(
                    wait_time).EndDict().Build().AsDict();
            items.push_back(wait_item);

            total_time += edge.total_time;
            json::Dict bus_item = json::Builder{}.StartDict().Key("type"s).Value("Bus"s).Key("bus"s).Value(
                    std::string(edge.bus_name)).Key("span_count"s).Value(edge.span_count).Key("time"s).Value(
                    edge.total_time - wait_time).EndDict().Build().AsDict();
            items.push_back(bus_item);
        }

        json_requests_result.push_back(json::Builder{}.StartDict().Key("request_id"s).Value(request_id).Key("total_time"s).Value(
                total_time).Key("items"s).Value(items).EndDict().Build());
    }
}

json::Document JsonReader::ParseRequest(transport_catalogue::TransportCatalogue& transport_catalogue_,
                                        std::optional<maper::Maper> maper_) {
    const auto& stat_requests_array = json_requests_->AsDict().at("stat_requests").AsArray();
    const json::Dict& routing_settings = json_requests_->AsDict().at("routing_settings"s).AsDict();
    transport_router::RouterSettings router_settings = AddRouterSettings(routing_settings);
    transport_router::TransportRouter transport_router_(transport_catalogue_, router_settings);
    json::Array json_requests_result;
    for (const auto& stat_request : stat_requests_array) {
        auto stat_request_map = stat_request.AsDict();
        int request_id = stat_request_map.at("id").AsInt();

        if (stat_request_map.at("type").AsString() == "Stop") {
            StopRequest(json_requests_result, transport_catalogue_, stat_request_map, request_id);
        } else if (stat_request_map.at("type").AsString() == "Bus") {
            BusRequest(json_requests_result, transport_catalogue_, stat_request_map, request_id);
        }
        else if (stat_request_map.at("type").AsString() == "Map") {
            MapRequest(maper_, json_requests_result, transport_catalogue_, request_id);
        } else if (stat_request_map.at("type").AsString() == "Route") {
            RouteRequest(json_requests_result, transport_router_, stat_request_map, request_id);
        }
    }
    return json::Document(json::Node(std::move(json_requests_result)));

}
