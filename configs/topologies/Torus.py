from m5.params import *
from m5.objects import *

from common import FileSystemConfig

from topologies.BaseTopology import SimpleTopology

# Creates a generic Mesh assuming an equal number of cache
# and directory controllers.
# XY routing is enforced (using link weights)
# to guarantee deadlock freedom.


class Torus(SimpleTopology):
    description = "Torus"

    def __init__(self, controllers):
        self.nodes = controllers

    # Makes a generic mesh
    # assuming an equal number of cache and directory cntrls

    def makeTopology(self, options, network, IntLink, ExtLink, Router):
        nodes = self.nodes

        num_cpus = options.num_cpus
        dims = [
            int(d) for d in options.dims.split(",")
        ]  # e.g., "4,4,4" -> [4, 4, 4]
        num_routers = 1
        for d in dims:
            num_routers *= d

        assert (
            num_routers == num_cpus
        ), "Number of routers must be equal to the number of CPUs"

        # default values for link latency and router latency.
        # Can be over-ridden on a per link/router basis
        link_latency = options.link_latency  # used by simple and garnet
        router_latency = options.router_latency  # only used by garnet

        # There must be an evenly divisible number of cntrls to routers
        # Also, obviously the number or rows must be <= the number of routers
        cntrls_per_router, remainder = divmod(len(nodes), num_routers)

        # Create the routers in the mesh
        routers = [
            Router(router_id=i, latency=router_latency)
            for i in range(num_routers)
        ]
        network.routers = routers

        # link counter to set unique link ids
        link_count = 0

        # Add all but the remainder nodes to the list of nodes to be uniformly
        # distributed across the network.
        network_nodes = []
        remainder_nodes = []
        for node_index in range(len(nodes)):
            if node_index < (len(nodes) - remainder):
                network_nodes.append(nodes[node_index])
            else:
                remainder_nodes.append(nodes[node_index])

        # Connect each node to the appropriate router
        ext_links = []
        for (i, n) in enumerate(network_nodes):
            cntrl_level, router_id = divmod(i, num_routers)
            assert cntrl_level < cntrls_per_router
            ext_links.append(
                ExtLink(
                    link_id=link_count,
                    ext_node=n,
                    int_node=routers[router_id],
                    latency=link_latency,
                )
            )
            link_count += 1

        # Connect the remainding nodes to router 0.  These should only be
        # DMA nodes.
        for (i, node) in enumerate(remainder_nodes):
            assert node.type == "DMA_Controller"
            assert i < remainder
            ext_links.append(
                ExtLink(
                    link_id=link_count,
                    ext_node=node,
                    int_node=routers[0],
                    latency=link_latency,
                )
            )
            link_count += 1

        network.ext_links = ext_links

        # Create the torus links.
        int_links = []

        def coordstoidx(coords):
            index = 0
            multiplier = 1
            for i, d in enumerate(coords):
                index += d * multiplier
                multiplier *= dims[i]
            return index

        def get_neighbor_coords(coords, dim, direction):
            """
            Get the coordinates of a neighbor in a specified dimension.
            """
            neighbor_coords = list(coords)
            neighbor_coords[dim] = (coords[dim] + direction) % dims[dim]
            return tuple(neighbor_coords)

        for router_index in range(num_routers):
            coords = []
            temp_index = router_index
            for d in dims:
                coords.append(temp_index % d)
                temp_index //= d
            coords = tuple(coords)

            # Create bidirectional links for each dimension
            for dim in range(len(dims)):
                neighbor_coords_pos = get_neighbor_coords(coords, dim, 1)
                neighbor_coords_neg = get_neighbor_coords(coords, dim, -1)

                pos_index = coordstoidx(neighbor_coords_pos)
                neg_index = coordstoidx(neighbor_coords_neg)

                int_links.append(
                    IntLink(
                        link_id=link_count,
                        src_node=routers[router_index],
                        dst_node=routers[pos_index],
                        src_outport=f"dim{dim}_pos",
                        dst_inport=f"dim{dim}_neg",
                        latency=link_latency,
                        weight=1,
                    )
                )
                link_count += 1

                int_links.append(
                    IntLink(
                        link_id=link_count,
                        src_node=routers[router_index],
                        dst_node=routers[neg_index],
                        src_outport=f"dim{dim}_neg",
                        dst_inport=f"dim{dim}_pos",
                        latency=link_latency,
                        weight=1,
                    )
                )
                link_count += 1

        network.int_links = int_links

    # Register nodes with filesystem
    def registerTopology(self, options):
        for i in range(options.num_cpus):
            FileSystemConfig.register_node(
                [i], MemorySize(options.mem_size) // options.num_cpus, i
            )
