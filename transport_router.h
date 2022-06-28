#pragma once

#include<unordered_map>

#include "transport_catalogue.h"
#include "router.h"

namespace transport_router {

    struct RouterSettings {
        int bus_wait_time_ = 0;
        double bus_velocity_ = 0;
    };

    struct RouteData {
        std::string_view bus_name_;
        double total_time_ = 1.0;
        int span_count_ = 1;
    };

    bool operator<(const RouteData& left, const RouteData& right);

    bool operator>(const RouteData& left, const RouteData& right);

    RouteData operator+(const RouteData& left, const RouteData& right);

    struct RouterInfo {
        std::string_view bus_name;
        std::string_view stop_from;
        double total_time = 0;
        int span_count = 0;
    };

    class TransportRouter {
    public:

        TransportRouter(transport_catalogue::TransportCatalogue& transport_catalogue, RouterSettings& router_settings);

        std::optional<std::vector<RouterInfo>> FindFastestRoute(const std::string from, const std::string to);

        const RouterSettings GetRouterSettings() const;

        graph::Edge<RouteData> BuildEdgeFromBus(const transport_catalogue::Bus& bus, size_t stop_from_index, size_t stop_to_index, double total_time);

        double ComputeTimeFromBus(const transport_catalogue::Bus& bus, int stop_from_index, int stop_to_index);

        void BuildIdContainers();

        void BuildGraph();

    private:
        const transport_catalogue::TransportCatalogue& transport_catalogue_;
        graph::DirectedWeightedGraph<RouteData> graph_;
        mutable std::unique_ptr<graph::Router<RouteData>> router_;
        std::unordered_map<size_t, const transport_catalogue::Stop*> stops_to_id;
        std::unordered_map<std::string_view, size_t> ids_to_stops;
        RouterSettings& router_settings_;
    };

}