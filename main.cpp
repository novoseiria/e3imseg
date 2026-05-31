// SPDX-FileCopyrightText: Copyright (C) Nile Jocson <novoseiria@gmail.com>
// SPDX-License-Identifier: MPL-2.0

#include <cstdlib>
#include <cmath>

#include <algorithm>
#include <exception>
#include <string>
#include <iostream>
#include <iomanip>
#include <stdexcept>
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

	auto print_table
	(
		std::string const &title,
		std::vector<std::pair<std::string, std::string>> const &rows
	) -> void
	{
		std::cout << title << "\n=========================\n";
		for (auto row: rows)
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
	int w;

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
		args.w = utils::parse_int(argv[4]);

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

		auto graph = PixelAdjGraph {};
		if (height == 0 || width == 0)
		{
			return graph;
		}

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



auto run(int argc, char *argv[]) -> void
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

	auto const image = ppm::load(args.input_filename);

	auto const graph = PixelAdjGraph::from_image(image);
	auto const vertex_count = graph.vertices.size();
	auto const edge_count = graph.edges.size();
	auto const min_weight = std::min_element(graph.edges.begin(), graph.edges.end())->weight;
	auto const max_weight = std::max_element(graph.edges.begin(), graph.edges.end())->weight;
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
