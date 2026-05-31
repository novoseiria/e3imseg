// SPDX-FileCopyrightText: Copyright (C) Nile Jocson <novoseiria@gmail.com>
// SPDX-License-Identifier: MPL-2.0

#include <string>
#include <iostream>



struct Args
{
	std::string input_file;
	std::string output_file;
	int k;
	int w;

public:
	static auto from_os_args(char *argv[]) -> Args
	{
		Args args;
		args.input_file = argv[1];
		args.output_file = argv[2];
		args.k = std::stoi(argv[3]);
		args.w = std::stoi(argv[4]);

		return args;
	}
};



auto main(int argc, char *argv[]) -> int
{
	auto const args = Args::from_os_args(argv);
	std::cout << args.input_file << '\n';
	std::cout << args.output_file << '\n';
	std::cout << args.k << '\n';
	std::cout << args.w << '\n';
}
