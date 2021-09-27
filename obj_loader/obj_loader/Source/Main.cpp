#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>

struct vec4
{
	float x, y, z, w;
};

struct vec2
{
	float u, v;
};

bool ProcessLine(const std::string& a_inLine, std::string& a_outKey, std::string& a_outValue)
{
	if (!a_inLine.empty())
	{
		//find the first character on the line that is not a space or a tab
		size_t keyStart = a_inLine.find_first_not_of("\t\r\n");
		//if key start is not valid
		if (keyStart == std::string::npos)
			return false;
		//starting from the key find the first character that is not a space ora tab
		size_t keyEnd = a_inLine.find_first_of("\t\r\n", keyStart);
		size_t valueStart = a_inLine.find_first_not_of(" \t\r\n", keyEnd);
		//find the end position for the value
		//from the end of the line find the last character that isn't space, tab, newline or return
		// +1 to include the last character itself
		size_t valueEnd = a_inLine.find_last_not_of("\t\n\r") + 1;
		//now that we have the start and end positions for the data use substring
		a_outKey = a_inLine.substr(keyStart, keyEnd - keyStart);
		if (valueStart == std::string::npos)
		{
			//if we get here then we had a line with a key and no data value
			// e.g. "#\n"
			a_outValue = "";
			return true;
		}
		a_outValue = a_inLine.substr(valueStart, valueEnd - valueStart);
		return true;
	}
	return false;
}

vec4 processVectorString(const std::string a_data) 
{
	//split the line data at each space character and store this as a float value within a glm::vec4
	std::stringstream iss(a_data);
	//create a zero vec4
	vec4 vecData = { 0.f, 0.f, 0.f, 0.f };
	int i = 0;
	//for loop to loop until iss cannot stream data into val
	for (std::string val; iss >> val; ++i)
	{
		//use std::string to float function
		float fVal = std::stof(val);
		//cast vec4 to float* to allow iteration through elements of vec4
		((float*)(&vecData))[i] = fVal;
	}
	return vecData;
}

int main(int argc, char* argv[])
{
	std::string filename = "obj_models/basic_box.OBJ";
	std::cout << "Attempting to open file: " << filename << std::endl;
	//use fstream to read file data
	std::fstream file;
	file.open(filename, std::ios_base::in | std::ios_base::binary);
	if (file.is_open())
	{
		std::cout << "Successfully Opened!" << std::endl;
		//attempt to read the maximum number of bytes available from the file
		file.ignore(std::numeric_limits<std::streamsize>::max());
		//the fstream gcounter will now be at the end of the file, gcount is a byte offset from 0
		//0 is the start of the file, or how many bytes file.ignore just read
		std::streamsize fileSize = file.gcount();
		//clear the EDF marker from being read
		file.clear();
		//reset the seekg back to the start of the file
		file.seekg(0, std::ios_base::beg);
		//write out the files size to console if it contains data
		if (fileSize != 0)
		{
			std::cout << std::fixed;
			std::cout << std::setprecision(2);
			std::cout << "File Size: " << fileSize / (float)1024 << "KB" << std::endl;
			//File is open and contain data
			//read each line of the file and display to the console
			std::string fileLine;
			//while the end of the file (EOF) token has not been read
			std::vector<vec4> vertexData;
			std::vector<vec4> normalData;
			std::vector<vec2> textureData;

			while (!file.eof())
			{
				if (std::getline(file, fileLine))
				{
					//if we get here we managed to read a line from the file
					std::string key;
					std::string value;
					if (ProcessLine(fileLine, key, value))
					{
						if (key == "#") //this is a comment line
						{
							std::cout << value << std::endl;
						}
						if (key == "v")
						{
							vec4 vertex = processVectorString(value);
							vertex.w = 1.f; //as this is postional data ensure that w component is set to 1
							vertexData.push_back(vertex);
						}
					}
				}
			}
			std::cout << "Processed " << vertexData.size() << " vertices in OBJ File" << std::endl;
		}
		else
		{
			std::cout << "File contains no data, closing file" << std::endl;
		}
		file.close();
	}
	else
	{
		std::cout << "Unable to open file " << filename << std::endl;
	}
	return EXIT_SUCCESS;

}


