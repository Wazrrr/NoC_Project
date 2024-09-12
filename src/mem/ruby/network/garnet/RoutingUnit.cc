/*
 * Copyright (c) 2008 Princeton University
 * Copyright (c) 2016 Georgia Institute of Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <random>
#include "mem/ruby/network/garnet/RoutingUnit.hh"

#include "base/cast.hh"
#include "base/compiler.hh"
#include "debug/RubyNetwork.hh"
#include "mem/ruby/network/garnet/InputUnit.hh"
#include "mem/ruby/network/garnet/OutputUnit.hh"
#include "mem/ruby/network/garnet/Router.hh"
#include "mem/ruby/slicc_interface/Message.hh"

namespace gem5
{

namespace ruby
{

namespace garnet
{

RoutingUnit::RoutingUnit(Router *router)
{
    m_router = router;
    m_routing_table.clear();
    m_weight_table.clear();
}

void
RoutingUnit::addRoute(std::vector<NetDest>& routing_table_entry)
{
    if (routing_table_entry.size() > m_routing_table.size()) {
        m_routing_table.resize(routing_table_entry.size());
    }
    for (int v = 0; v < routing_table_entry.size(); v++) {
        m_routing_table[v].push_back(routing_table_entry[v]);
    }
}

void
RoutingUnit::addWeight(int link_weight)
{
    m_weight_table.push_back(link_weight);
}

bool
RoutingUnit::supportsVnet(int vnet, std::vector<int> sVnets)
{
    // If all vnets are supported, return true
    if (sVnets.size() == 0) {
        return true;
    }

    // Find the vnet in the vector, return true
    if (std::find(sVnets.begin(), sVnets.end(), vnet) != sVnets.end()) {
        return true;
    }

    // Not supported vnet
    return false;
}

/*
 * This is the default routing algorithm in garnet.
 * The routing table is populated during topology creation.
 * Routes can be biased via weight assignments in the topology file.
 * Correct weight assignments are critical to provide deadlock avoidance.
 */
int
RoutingUnit::lookupRoutingTable(int vnet, NetDest msg_destination)
{
    // First find all possible output link candidates
    // For ordered vnet, just choose the first
    // (to make sure different packets don't choose different routes)
    // For unordered vnet, randomly choose any of the links
    // To have a strict ordering between links, they should be given
    // different weights in the topology file

    int output_link = -1;
    int min_weight = INFINITE_;
    std::vector<int> output_link_candidates;
    int num_candidates = 0;

    // Identify the minimum weight among the candidate output links
    for (int link = 0; link < m_routing_table[vnet].size(); link++) {
        if (msg_destination.intersectionIsNotEmpty(
            m_routing_table[vnet][link])) {

        if (m_weight_table[link] <= min_weight)
            min_weight = m_weight_table[link];
        }
    }

    // Collect all candidate output links with this minimum weight
    for (int link = 0; link < m_routing_table[vnet].size(); link++) {
        if (msg_destination.intersectionIsNotEmpty(
            m_routing_table[vnet][link])) {

            if (m_weight_table[link] == min_weight) {
                num_candidates++;
                output_link_candidates.push_back(link);
            }
        }
    }

    if (output_link_candidates.size() == 0) {
        fatal("Fatal Error:: No Route exists from this Router.");
        exit(0);
    }

    // Randomly select any candidate output link
    int candidate = 0;
    if (!(m_router->get_net_ptr())->isVNetOrdered(vnet))
        candidate = rand() % num_candidates;

    output_link = output_link_candidates.at(candidate);
    return output_link;
}


void
RoutingUnit::addInDirection(PortDirection inport_dirn, int inport_idx)
{
    m_inports_dirn2idx[inport_dirn] = inport_idx;
    m_inports_idx2dirn[inport_idx]  = inport_dirn;
}

void
RoutingUnit::addOutDirection(PortDirection outport_dirn, int outport_idx)
{
    m_outports_dirn2idx[outport_dirn] = outport_idx;
    m_outports_idx2dirn[outport_idx]  = outport_dirn;
}

// outportCompute() is called by the InputUnit
// It calls the routing table by default.
// A template for adaptive topology-specific routing algorithm
// implementations using port directions rather than a static routing
// table is provided here.

int
RoutingUnit::outportCompute(RouteInfo route, int inport,
                            PortDirection inport_dirn)
{
    int outport = -1;

    if (route.dest_router == m_router->get_id()) {

        // Multiple NIs may be connected to this router,
        // all with output port direction = "Local"
        // Get exact outport id from table
        outport = lookupRoutingTable(route.vnet, route.net_dest);
        return outport;
    }

    // Routing Algorithm set in GarnetNetwork.py
    // Can be over-ridden from command line using --routing-algorithm = 1
    RoutingAlgorithm routing_algorithm =
        (RoutingAlgorithm) m_router->get_net_ptr()->getRoutingAlgorithm();

    switch (routing_algorithm) {
        case TABLE_:  outport =
            lookupRoutingTable(route.vnet, route.net_dest); break;
        case XY_:     outport =
            outportComputeXY(route, inport, inport_dirn); break;
        // any custom algorithm
        case Ring_: outport =
            outportComputeRing(route, inport, inport_dirn); break;
        case Dor_: outport =
            outportComputeTorus_Shortest_Path(route, inport, inport_dirn); break;
        case Goal_: outport =
            outportComputeTorus(route, inport, inport_dirn); break;
        case Min_AD_: outport =
            outportComputeMINAD(route, inport, inport_dirn); break;
        default: outport =
            lookupRoutingTable(route.vnet, route.net_dest); break;
    }

    assert(outport != -1);
    return outport;
}

// XY routing implemented using port directions
// Only for reference purpose in a Mesh
// By default Garnet uses the routing table
int
RoutingUnit::outportComputeXY(RouteInfo route,
                              int inport,
                              PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";

    [[maybe_unused]] int num_rows = m_router->get_net_ptr()->getNumRows();
    int num_cols = m_router->get_net_ptr()->getNumCols();
    assert(num_rows > 0 && num_cols > 0);

    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = my_id / num_cols;

    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = dest_id / num_cols;

    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);

    bool x_dirn = (dest_x >= my_x);
    bool y_dirn = (dest_y >= my_y);

    // already checked that in outportCompute() function
    assert(!(x_hops == 0 && y_hops == 0));

    if (x_hops > 0) {
        if (x_dirn) {
            assert(inport_dirn == "Local" || inport_dirn == "West");
            outport_dirn = "East";
        } else {
            assert(inport_dirn == "Local" || inport_dirn == "East");
            outport_dirn = "West";
        }
    } else if (y_hops > 0) {
        if (y_dirn) {
            // "Local" or "South" or "West" or "East"
            assert(inport_dirn != "North");
            outport_dirn = "North";
        } else {
            // "Local" or "North" or "West" or "East"
            assert(inport_dirn != "South");
            outport_dirn = "South";
        }
    } else {
        // x_hops == 0 and y_hops == 0
        // this is not possible
        // already checked that in outportCompute() function
        panic("x_hops == y_hops == 0");
    }

    return m_outports_dirn2idx[outport_dirn];
}

// Template for implementing custom routing algorithm
// using port directions. (Example adaptive)
RouteInfo
RoutingUnit::create_routeinfo(RouteInfo original_routeinfo, int new_outport)
{
    RouteInfo new_routeinfo = original_routeinfo;
    PortDirection outport_direction = m_outports_idx2dirn[new_outport];

    std::string dims_str = m_router->get_net_ptr()->getTorusDims();
    std::vector<int> dimensions;
    std::stringstream ss(dims_str);
    std::string token;

    while (std::getline(ss, token, ',')) {
        int dim = std::stoi(token);
        assert(dim > 0);
        dimensions.push_back(dim);
    }
    int my_id = m_router->get_id();
    std::vector<int> my_coords = index_to_coords(my_id, dimensions);
    //update the is_torus_dims_checkpoint
    for (int i = 0; i < my_coords.size(); i++) {
        if (my_coords[i] == 0 && outport_direction == "dim"+std::to_string(i)+"_neg") {
            new_routeinfo.is_torus_dims_checkpoint[i] = true;
        }
        else if (my_coords[i] == dimensions[i]-1 && outport_direction == "dim"+std::to_string(i)+"_pos") {
            new_routeinfo.is_torus_dims_checkpoint[i] = true;
        }
    }
    return new_routeinfo;
}

int
RoutingUnit::outportComputeRing(RouteInfo route,
                                 int inport,
                                 PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";

    int num_routers = m_router->get_net_ptr()->getNumRouters();
    int my_id = m_router->get_id();
    int dest_id = route.dest_router;
    int hops = (dest_id - my_id + num_routers) % num_routers;

    if (hops >= num_routers/2) {
        outport_dirn = "West";
    }
    else {
        outport_dirn = "East";
        // panic("%s placeholder executed", __FUNCTION__);
    }

    return m_outports_dirn2idx[outport_dirn];
}

//function: index to coordinates
std::vector<int>
RoutingUnit::index_to_coords(int index, const std::vector<int>& dims) {
    std::vector<int> coords(dims.size());  // Initialize the coords vector with the same size as dims
    for (int i = 0; i < dims.size(); i++) {
        coords[i] = index % dims[i];  // Calculate the coordinate for dimension i
        index /= dims[i];  // Update the index for the next dimension
    }
    return coords;
}

int
RoutingUnit::outportComputeTorus_Shortest_Path(RouteInfo route,
                                 int inport,
                                 PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";

    std::string dims_str = m_router->get_net_ptr()->getTorusDims();
    std::vector<int> dimensions;
    std::stringstream ss(dims_str);
    std::string token;

    while (std::getline(ss, token, ',')) {
        int dim = std::stoi(token);
        assert(dim > 0);
        dimensions.push_back(dim);
    }


    int num_dims = dimensions.size();

    // convert index to coordinates
    int my_id = m_router->get_id();
    std::vector<int> my_coords = index_to_coords(my_id, dimensions);

    int dest_id = route.dest_router;
    std::vector<int> dest_coords = index_to_coords(dest_id, dimensions);
    int pos_hops = 0;
    // go each dimensions sequentially, just go pos direction
    for (int i = 0; i < num_dims; ++i) {
        pos_hops = (dest_coords[i] - my_coords[i] + dimensions[i]) % dimensions[i];
        if (pos_hops == 0) {
            continue;
        }
        else if (pos_hops < dimensions[i]/2) {
            outport_dirn = "dim" + std::to_string(i) + "_pos";
        }
        else {
            outport_dirn = "dim" + std::to_string(i) + "_neg";
        }
    }

    return m_outports_dirn2idx[outport_dirn];
}

int
RoutingUnit::outportComputeTorus(RouteInfo route,
                                 int inport,
                                 PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";

    std::string dims_str = m_router->get_net_ptr()->getTorusDims();
    std::vector<int> dimensions;
    std::stringstream ss(dims_str);
    std::string token;

    while (std::getline(ss, token, ',')) {
        int dim = std::stoi(token);
        assert(dim > 0);
        dimensions.push_back(dim);
    }

    int num_dims = dimensions.size();

    // convert index to coordinates
    int my_id = m_router->get_id();
    std::vector<int> my_coords = index_to_coords(my_id, dimensions);

    int dest_id = route.dest_router;
    std::vector<int> dest_coords = index_to_coords(dest_id, dimensions);

    //Choose the directions when initialzing
    if (route.directions.empty()) {
        std::vector<double> pos_probability(num_dims);
        for (int i = 0; i < num_dims; i++) {
            pos_probability[i] = 1.0 - static_cast<double>((dest_coords[i] - my_coords[i] + dimensions[i]) % dimensions[i]) / static_cast<double>(dimensions[i]);
        }
        // Create a random number generator
        std::random_device rd;  // Obtain a random number from hardware
        std::mt19937 gen(rd()); // Seed the generator
        std::uniform_real_distribution<> dis(0.0, 1.0);

        route.directions.resize(num_dims); // Resize to hold num_dims elements
        for (int i = 0; i < num_dims; ++i) {
            if (pos_probability[i] == 1) {route.directions[i]=0; continue;}
            double random_number = dis(gen);
            route.directions[i] = (random_number < pos_probability[i]) ? 1 : -1;

            // if (pos_probability[i] >0.5) route.directions[i] = 1;
            // else route.directions[i] = -1;// for testing

            if (pos_probability[i] >0.5 && route.directions[i] == 1) {route.is_minimal_torus = true;}
            else if (pos_probability[i] <0.5 && route.directions[i] == -1) {route.is_minimal_torus = true;}

        }
    }

    std::vector<unsigned> check_outports_idxs;
    for (int i = 0; i < num_dims; ++i) {
        if (dest_coords[i] == my_coords[i]) {
            route.directions[i] = 0;
            continue;
        }
        PortDirection temp_outport;
        if (route.directions[i] == 1) {
            temp_outport = "dim" + std::to_string(i) + "_pos";
        } else if (route.directions[i] == -1) {
            temp_outport = "dim" + std::to_string(i) + "_neg";
        }
        check_outports_idxs.push_back(m_outports_dirn2idx[temp_outport]);
    }

    // check these outports, count their vcs and pick the one with minimum occupation in vc.
    int max_vc_free = -1;
    int outport = check_outports_idxs[0];
    RoutingAlgorithm routing_algorithm =
        (RoutingAlgorithm) m_router->get_net_ptr()->getRoutingAlgorithm();
    for (int i = 0; i < check_outports_idxs.size(); ++i) {
        auto output_unit = m_router->getOutputUnit(check_outports_idxs[i]);
        int vc_free = output_unit->count_free_vc(route.vnet,false,false,true,route.is_torus_dims_checkpoint, route.is_minimal_torus, routing_algorithm);
        if (vc_free > max_vc_free) {
            max_vc_free = vc_free;
            outport = check_outports_idxs[i];
        }
    }

    return outport;
}

int
RoutingUnit::outportComputeMINAD(RouteInfo route,
                                 int inport,
                                 PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";

    std::string dims_str = m_router->get_net_ptr()->getTorusDims();
    std::vector<int> dimensions;
    std::stringstream ss(dims_str);
    std::string token;

    while (std::getline(ss, token, ',')) {
        int dim = std::stoi(token);
        assert(dim > 0);
        dimensions.push_back(dim);
    }

    int num_dims = dimensions.size();

    // convert index to coordinates
    int my_id = m_router->get_id();
    std::vector<int> my_coords = index_to_coords(my_id, dimensions);

    int dest_id = route.dest_router;
    std::vector<int> dest_coords = index_to_coords(dest_id, dimensions);

    //Choose the directions when initialzing
    if (route.directions.empty()) {
        std::vector<double> pos_probability(num_dims);
        for (int i = 0; i < num_dims; i++) {
            pos_probability[i] = 1.0 - static_cast<double>((dest_coords[i] - my_coords[i] + dimensions[i]) % dimensions[i]) / static_cast<double>(dimensions[i]);
        }

        route.directions.resize(num_dims); // Resize to hold num_dims elements
        for (int i = 0; i < num_dims; ++i) {
            if (pos_probability[i] == 1) {route.directions[i]=0; continue;}

            if (pos_probability[i] > 0.5) route.directions[i] = 1;
            else route.directions[i] = -1;// min ad

            route.is_minimal_torus = true;

        }
    }

    std::vector<unsigned> check_outports_idxs;
    for (int i = 0; i < num_dims; ++i) {
        if (dest_coords[i] == my_coords[i]) {
            route.directions[i] = 0;
            continue;
        }
        PortDirection temp_outport;
        if (route.directions[i] == 1) {
            temp_outport = "dim" + std::to_string(i) + "_pos";
        } else if (route.directions[i] == -1) {
            temp_outport = "dim" + std::to_string(i) + "_neg";
        }
        check_outports_idxs.push_back(m_outports_dirn2idx[temp_outport]);
    }

    // check these outports, count their vcs and pick the one with minimum occupation in vc.
    int max_vc_free = -1;
    int outport = check_outports_idxs[0];
    RoutingAlgorithm routing_algorithm =
        (RoutingAlgorithm) m_router->get_net_ptr()->getRoutingAlgorithm();
    for (int i = 0; i < check_outports_idxs.size(); ++i) {
        auto output_unit = m_router->getOutputUnit(check_outports_idxs[i]);
        int vc_free = output_unit->count_free_vc(route.vnet,false,false,true,route.is_torus_dims_checkpoint, route.is_minimal_torus, routing_algorithm);
        if (vc_free > max_vc_free) {
            max_vc_free = vc_free;
            outport = check_outports_idxs[i];
        }
    }

    return outport;
}

} // namespace garnet
} // namespace ruby
} // namespace gem5
