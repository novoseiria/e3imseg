// SPDX-FileCopyrightText: Copyright (C) Nile Jocson <novoseiria@gmail.com>
// SPDX-License-Identifier: MPL-2.0

#include <cstddef>
#include <cstdlib>
#include <cmath>

#include <algorithm>
#include <exception>
#include <numeric>
#include <string>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <utility>
#include <vector>

#include "ppm.h"



namespace error
{
	auto load(std::string const &filename) -> std::runtime_error
	{
		return std::runtime_error("Failed to load image " + filename);
	}

	auto save(std::string const &filename) -> std::runtime_error
	{
		return std::runtime_error("Failed to save image to " + filename);
	}

	auto invalid_arg_count(int const expected, int const actual)
		-> std::runtime_error
	{
		return std::runtime_error
		(
			"Invalid argument count: expected "
			+ std::to_string(expected)
			+ ", found "
			+ std::to_string(actual)
		);
	}

	auto parse_int(std::string const &value) -> std::runtime_error
	{
		return std::runtime_error
		(
			"Failed to parse value '"
			+ value
			+ "' as int"
		);
	}

	auto parse_double(std::string const &value) -> std::runtime_error
	{
		return std::runtime_error
		(
			"Failed to parse value '"
			+ value
			+ "' as double"
		);
	}

	auto empty_image(std::string const &filename) -> std::runtime_error
	{
		return std::runtime_error
		(
			"Failed to load "
			+ filename
			+ ": empty image");
	}

	auto no_edges() -> std::runtime_error
	{
		return std::runtime_error("Cannot generate a pixel adjacency graph from a 1x1 image");
	}
}



namespace utils
{
	auto parse_int(std::string const &value) -> int
	{
		try
		{
			return std::stoi(value);
		}
		catch (std::exception const &e)
		{
			throw error::parse_int(value);
		}
	}

	auto parse_double(std::string const &value) -> double
	{
		try
		{
			return std::stod(value);
		}
		catch (std::exception const &e)
		{
			throw error::parse_double(value);
		}
	}

	auto print_table
	(
		std::string const &title,
		std::vector<std::pair<std::string, std::string>> const &rows
	) -> void
	{
		std::cout << title << "\n=========================\n";
		for (auto const &row: rows)
		{
			std::cout
				<< std::left
				<< std::setw(12)
				<< row.first
				<< ": "
				<< row.second
				<< '\n';
		}
		std::cout << '\n';
	}
}



namespace ppm
{
	auto load(std::string const &filename) -> PPMImage
	{
		auto image = PPMImage();

		if (!image.load(filename))
		{
			throw error::load(filename);
		}
		if (image.width == 0 || image.height == 0)
		{
			throw error::empty_image(filename);
		}

		return image;
	}

	auto save(PPMImage &image, std::string const &filename) -> void
	{
		if (!image.save(filename))
		{
			throw error::save(filename);
		}
	}
}



namespace pixel
{
	auto rgb_diff(Pixel const &a, Pixel const &b) -> double
	{
		auto const dr = std::abs(static_cast<int>(a.r) - static_cast<int>(b.r));
		auto const dg = std::abs(static_cast<int>(a.g) - static_cast<int>(b.g));
		auto const db = std::abs(static_cast<int>(a.b) - static_cast<int>(b.b));

		return (dr + dg + db)/3.0;
	}

	auto hue(Pixel const &pixel) -> double
	{
		auto const r = pixel.r;
		auto const g = pixel.g;
		auto const b = pixel.b;

		auto const max = std::max({ r, g, b });
		auto const min = std::min({ r, g, b });

		if (max == min)
		{
			return 0.0;
		}

		auto x = 0;
		auto y = 0;
		auto offset = 0.0;

		if (max == r)
		{
			x = g;
			y = b;
		}
		else if (max == g)
		{
			x = b;
			y = r;
			offset = 120.0;
		}
		else if (max == b)
		{
			x = r;
			y = g;
			offset = 240.0;
		}

		auto const hue = offset + 60.0 * (x - y) / (max - min);

		return hue < 0
			? hue + 360
			: hue;
	}

	auto hue_diff(Pixel const &a, Pixel const &b) -> double
	{
		auto const diff = std::abs(hue(a) - hue(b));

		return diff > 180
			? 360 - diff
			: diff;
	}

	auto weight(Pixel const &a, Pixel const &b) -> double
	{
		return rgb_diff(a, b) / 255 * (hue_diff(a, b) / 360 + 0.5);
	}
}



struct Args
{
	std::string input_filename;
	std::string output_filename;
	int k;
	double w;

public:
	static auto from_os_args(int argc, char *argv[]) -> Args
	{
		if (argc != 5)
		{
			throw error::invalid_arg_count(4, argc - 1);
		}

		auto args = Args {};
		args.input_filename = argv[1];
		args.output_filename = argv[2];
		args.k = utils::parse_int(argv[3]);
		args.w = utils::parse_double(argv[4]);

		return args;
	}
};



struct Edge
{
	std::size_t u;
	std::size_t v;
	double weight;

public:
	friend auto operator<(Edge const &l, Edge const &r) -> bool
	{
		return l.weight < r.weight;
	}
};

struct PixelAdjGraph
{
	std::vector<Pixel> vertices;
	std::vector<Edge> edges;

public:
	static auto from_image(PPMImage const &image) -> PixelAdjGraph
	{
		auto const width  = static_cast<std::size_t>(image.width);
		auto const height = static_cast<std::size_t>(image.height);

		if (width == 1 && height == 1)
		{
			throw error::no_edges();
		}

		auto graph = PixelAdjGraph {};

		graph.vertices.reserve(width * height);
		for (auto row = std::size_t { 0 }; row < height; ++row)
		{
			for (auto col = std::size_t { 0 }; col < width; ++col)
			{
				graph.vertices.push_back(image[row][col]);
			}
		}

		graph.edges.reserve(height * (width - 1) + width * (height - 1));
		for (auto row = std::size_t { 0 }; row < height; ++row)
		{
			for (auto col = std::size_t { 0 }; col < width; ++col)
			{
				auto const u = row * width + col;

				if (col + 1 < width)
				{
					auto const v = u + 1;
					graph.edges.push_back(Edge
					{
						u,
						v,
						pixel::weight(graph.vertices[u], graph.vertices[v])
					});
				}

				if (row + 1 < height)
				{
					auto const v = (row + 1) * width + col;
					graph.edges.push_back(Edge
					{
						u,
						v,
						pixel::weight(graph.vertices[u], graph.vertices[v])
					});
				}
			}
		}

		return graph;
	}
};



class DisjointSet
{
public:
	static auto from_size(std::size_t const size) -> DisjointSet
	{
		auto ds = DisjointSet {};

		ds.m_parents.resize(size);
		std::iota(ds.m_parents.begin(), ds.m_parents.end(), 0);

		ds.m_sizes.resize(size, 1);
		ds.m_set_count = size;

		return ds;
	}

	auto root(std::size_t const index) -> std::size_t
	{
		if (m_parents[index] == index)
		{
			return index;
		}

		auto const root_index = root(m_parents[index]);
		m_parents[index] = root_index;

		return root_index;
	}

	auto merge(std::size_t const a, std::size_t const b) -> bool
	{
		auto root_a = root(a);
		auto root_b = root(b);

		if (root_a == root_b)
		{
			return false;
		}

		if (m_sizes[root_a] < m_sizes[root_b])
		{
			std::swap(root_a, root_b);
		}

		m_parents[root_b] = root_a;
		m_sizes[root_a] += m_sizes[root_b];
		--m_set_count;

		return true;
	}

	auto set_count() const -> std::size_t
	{
		return m_set_count;
	}

private:
	std::vector<std::size_t> m_parents;
	std::vector<std::size_t> m_sizes;
	std::size_t m_set_count;
};



namespace graph
{
	struct BoruvkaResult
	{
		std::vector<Edge> mst;
		std::size_t num_iterations;
	};

	struct SegmentationResult
	{
		DisjointSet ds;
		std::size_t num_iterations;
	};

	auto candidate_edges
	(
		PixelAdjGraph const &graph,
		DisjointSet &ds
	) -> std::vector<Edge const *>
	{
		auto cheapest = std::vector<Edge const *>(graph.vertices.size(), nullptr);

		for (auto const &edge: graph.edges)
		{
			auto const root_u = ds.root(edge.u);
			auto const root_v = ds.root(edge.v);

			if (root_u == root_v)
			{
				continue;
			}

			if (cheapest[root_u] == nullptr || edge.weight < cheapest[root_u] -> weight)
			{
				cheapest[root_u] = &edge;
			}

			if (cheapest[root_v] == nullptr || edge.weight < cheapest[root_v] -> weight)
			{
				cheapest[root_v] = &edge;
			}
		}

		auto candidates = std::vector<Edge const *> {};
		for (auto const edge: cheapest)
		{
			if (edge != nullptr)
			{
				candidates.push_back(edge);
			}
		}

		return candidates;
	}

	auto boruvka(PixelAdjGraph const &graph) -> BoruvkaResult
	{
		auto mst = std::vector<Edge> {};
		mst.reserve(graph.vertices.size() - 1);

		auto ds = DisjointSet::from_size(graph.vertices.size());

		auto num_iterations = std::size_t { 0 };
		while (ds.set_count() > 1)
		{
			++num_iterations;

			for (auto const candidate: candidate_edges(graph, ds))
			{
				if (ds.merge(candidate->u, candidate->v))
				{
					mst.push_back(*candidate);
				}
			}
		}

		return BoruvkaResult { std::move(mst), num_iterations };
	}

	auto segment(PixelAdjGraph const &graph, int const k, double const w) -> SegmentationResult
	{
		auto ds = DisjointSet::from_size(graph.vertices.size());

		auto num_iterations = std::size_t { 0 };
		while (ds.set_count() > k)
		{
			++num_iterations;

			auto candidates = candidate_edges(graph, ds);
			std::sort(candidates.begin(), candidates.end(), [](Edge const *a, Edge const *b) -> bool
			{
				return a->weight < b->weight;
			});

			auto merged_count = std::size_t { 0 };

			for (auto const edge: candidates)
			{
				if (ds.set_count() <= k)
				{
					break;
				}

				if (edge->weight < w && ds.merge(edge->u, edge->v))
				{
					++merged_count;
				}
			}

			if (merged_count == 0)
			{
				break;
			}
		}

		return SegmentationResult { std::move(ds), num_iterations };
	}
}



auto preliminary(int argc, char *argv[]) -> Args
{
	auto const args = Args::from_os_args(argc, argv);

	utils::print_table
	(
		"ARGS",
		{
			{ "Input File" , args.input_filename    },
			{ "Output File", args.output_filename   },
			{ "K"          , std::to_string(args.k) },
			{ "W"          , std::to_string(args.w) }
		}
	);

	return args;
}

auto subtask_1(std::string const &input_filename) -> PixelAdjGraph
{
	auto const image = ppm::load(input_filename);

	auto const graph = PixelAdjGraph::from_image(image);
	auto const vertex_count = graph.vertices.size();
	auto const edge_count = graph.edges.size();

	auto const minmax_it = std::minmax_element(graph.edges.begin(), graph.edges.end());
	auto const min_weight = minmax_it.first->weight;
	auto const max_weight = minmax_it.second->weight;

	utils::print_table
	(
		"SUBTASK 1",
		{
			{ "Vertex Count", std::to_string(vertex_count) },
			{ "Edge Count"  , std::to_string(edge_count)   },
			{ "Min Weight"  , std::to_string(min_weight)   },
			{ "Max Weight"  , std::to_string(max_weight)   }
		}
	);

	return graph;
}

auto subtask_2(PixelAdjGraph const &graph) -> std::vector<Edge>
{
	auto const result = graph::boruvka(graph);

	auto const &mst = result.mst;
	auto const num_iterations = result.num_iterations;

	auto const edge_count = mst.size();
	auto total_weight = 0.0;
	for (auto const &edge: mst)
	{
		total_weight += edge.weight;
	}

	utils::print_table
	(
		"SUBTASK 2",
		{
			{ "# of Iters." , std::to_string(num_iterations) },
			{ "Edge Count"  , std::to_string(edge_count)     },
			{ "Total Weight", std::to_string(total_weight)   }
		}
	);

	return mst;
}

auto subtask_3(PixelAdjGraph const &graph, int const k, double const w) -> DisjointSet
{
	auto const result = graph::segment(graph, k, w);

	auto const &ds = result.ds;
	auto const num_iterations = result.num_iterations;

	utils::print_table
	(
		"SUBTASK 3",
		{
			{ "# of Iters.", std::to_string(num_iterations) }
		}
	);

	return ds;
}



auto run(int argc, char *argv[]) -> void
{
	auto const args = preliminary(argc, argv);
	auto const graph = subtask_1(args.input_filename);
	auto const mst = subtask_2(graph);
	auto const ds = subtask_3(graph, args.k, args.w);
}

auto main(int argc, char *argv[]) -> int
{
	try
	{
		run(argc, argv);
		return EXIT_SUCCESS;
	}
	catch (std::exception const &e)
	{
		std::cerr << "e3imseg: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}
