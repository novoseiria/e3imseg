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

		auto const hue = offset + 60.0*(x - y)/(max - min);

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

		Args args;
		args.input_filename = argv[1];
		args.output_filename = argv[2];
		args.k = utils::parse_int(argv[3]);
		args.w = utils::parse_int(argv[4]);

		return args;
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
