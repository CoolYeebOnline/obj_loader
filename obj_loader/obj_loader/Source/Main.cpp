#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <map>

std::string g_filePath;

#pragma region VECTOR DATA

struct vec4
{
	float x, y, z, w;
};

struct vec2
{
	float u, v;
};

struct OBJVertex
{
	vec4 vertex;
	vec4 normal;
	vec2 uvCoord;
};

struct OBJMaterial
{
	std::string name;
	vec4		kA;	//Ambient Light Colour - alpha component stores optical density (Ni)(Refraction Index 0.001 - 10)
	vec4		kD; //Diffuse Light Colour - alpha component stores dissolve (d)(0-1)
	vec4		kS; //Specular Light Colour (exponent stored in alpha)
};


#pragma endregion

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

std::vector<std::string> splitStringAtCharacter(std::string data, char a_character)
{
	std::vector<std::string> lineData;
	std::stringstream iss(data);
	std::string lineSegment;
	//providing a character to the getline function splits the line at occurances of that character
	while (std::getline(iss, lineSegment, a_character))
	{
		//push each line segment into a vector
		lineData.push_back(lineSegment);
	}
	return lineData;
}

OBJVertex processFaceData(std::string a_faceData, std::vector<vec4>& a_vertexArray,
	std::vector<vec4>& a_normalArray, std::vector<vec2>& a_uvArray)
{
	std::vector<std::string> vertexIndices = splitStringAtCharacter(a_faceData, '/');
	//a simple local structure to hold face triplet data as integer values
	typedef struct objFaceTriplet { int32_t v,vn,vt; }objFaceTriplet;
	//instance of objFaceTriplet struct
	objFaceTriplet ft = { 0, 0, 0 };
	//parse vertex indices as integer values to look up in vertex/normal/uv array data
	ft.v = std::stoi(vertexIndices[0]);
	//if ise is >= 2 then there is additional information outside of vertex data
	if (vertexIndices.size() >= 2) {
		//if size of element 1 is greater than 0 then UV coord information present
		if (vertexIndices[1].size() > 0)
		{
			ft.vt = std::stoi(vertexIndices[1]);
		}
		//if size is greater than 3 then there is normal data present
		if (vertexIndices.size() >= 3)
		{
			ft.vn = std::stoi(vertexIndices[2]);
		}
	}
	//now that face index values have been processed retrieve actual data from vertex arrays
	OBJVertex currentVertex;
	currentVertex.vertex = a_vertexArray[size_t(ft.v) - 1];
	if (ft.vn != 0)
	{
		currentVertex.normal = a_normalArray[size_t(ft.v) - 1];
	}
	if (ft.vt != 0)
	{
		currentVertex.uvCoord = a_uvArray[size_t(ft.vt) - 1];
	}
	return currentVertex;
}

void LoadMaterialLibrary(std::string a_mtllib, std::vector<OBJMaterial>& a_materials)
{
	std::string matFile = g_filePath + a_mtllib;
	std::cout << "Attempting to load material file: " << matFile << std::endl;
	//get an fstream to read in the file data
	std::fstream file;
	file.open(matFile, std::ios_base::in | std::ios_base::binary);
	//test to see if the file has opened incorrectly
	if (file.is_open())
	{
		std::cout << "Material Library Successfully Opened" << std::endl;
		//success file has been opened, verify contents of file -- ie check that file is not zero length
		file.ignore(std::numeric_limits<std::streamsize>::max());
		std::streamsize fileSize = file.gcount();
		file.clear();
		file.seekg(0, std::ios_base::beg);
		if (fileSize == 0)
		{
			std::cout << "File contains no data, closing file" << std::endl;
			file.close();
		}
		std::cout << "Material File size: " << fileSize / 1024 << "KB" << std::endl;
		//variable to store file data as it is read line by line
		std::string fileLine;
		//pointer to a material object (the one at the end of the materials vector)
		OBJMaterial* currentMaterial = nullptr;

		while (!file.eof()) 
		{
			if (std::getline(file, fileLine))
			{
				std::string key;
				std::string value;
				if (ProcessLine(fileLine, key, value))
				{
					if (key == "#")//this is a comment line
					{
						std::cout << value << std::endl;
						continue;
					}
					if (key == "newmtl")
					{
						std::cout << "New Material found: " << value << std::endl;
						a_materials.push_back(OBJMaterial());
						currentMaterial = &(a_materials.back());
						currentMaterial->name = value;
						continue;
					}
					if (key == "Ns")
					{
						if (currentMaterial != nullptr)
						{
							//NS is guarenteed to be a single float value
							currentMaterial->kS.w - std::stof(value);
						}
						continue;
					}
					if (key == "Ka")
					{
						if (currentMaterial != nullptr)
						{
							//process kA as vetor string
							float kAd = currentMaterial ->kA.w; //store alpha channel as may contain refractive index
							currentMaterial->kA = processVectorString(value);
							currentMaterial->kA.w = kAd;
						}
						continue;
					}
					if (key == "Kd")
					{
						if (currentMaterial != nullptr)
						{
							//process kD as vector string
							float kDa = currentMaterial->kS.w; //store alpha as may contain specular component
							currentMaterial->kD = processVectorString(value);
							currentMaterial->kD.w = kDa;
						}
						continue;
					}
					if (key == "Ks")
					{
						if (currentMaterial != nullptr)
						{
							//process Ks as vector string
							float kSa = currentMaterial->kS.w; //store alpha as may contain specular component
							currentMaterial->kS = processVectorString(value);
							currentMaterial->kS.w = kSa;
						}
						continue;
					}
					if (key == "Ke")
					{
						//KE is for emissive properties
						//we will not need to support this for our purposes
						continue;
					}
					if (key == "Ni")
					{
						if (currentMaterial != nullptr)
						{
							//this is the refractive index of the mesh (how light bends as it passes through the material)
							//we will store this in the alpha component of the ambient light values (kA)
							currentMaterial->kA.w = std::stof(value);
						}
						continue;
					}
					if (key == "d" || key == "Tr") //Transparency/Opacity Tr = 1 -d
					{
						if (currentMaterial != nullptr)
						{
							//this is the dissolve or alpha value of the material we will store this in the kD alpha channel
							currentMaterial->kD.w = std::stof(value);
							if (key == "Tr")
							{
								currentMaterial->kD.w = 1.f - currentMaterial->kD.w;
							}
						}
						continue;
					}
					if (key == "illum")
					{
						//illum describes the illumination model used to light the model.
						//ignore this for now as we will light the scene our own way
						continue;
					}
				}
			}
		}
		file.close();
	}
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
		//Get File Path information
		g_filePath = filename;
		size_t path_end = g_filePath.find_last_of("/\\");
		if (path_end != std::string::npos)
		{
			g_filePath = g_filePath.substr(0, path_end + 1);
		}
		else
		{
			g_filePath = "";
		}
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
			std::map<std::string, int32_t> faceIndexMap;
			std::vector<vec4> vertexData;
			std::vector<vec4> normalData;
			std::vector<vec2> textureData;
			std::vector<OBJVertex> meshData;
			std::vector<uint32_t> meshIndices;
			std::vector<OBJMaterial> materials;
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
						if (key == "mtllib")
						{
							std::cout << "Material File: " << value << std::endl;
							//Load in Material file so that materials can be used as required
							LoadMaterialLibrary(value, materials);
						}
						if (key == "v")
						{
							vec4 vertex = processVectorString(value);
							vertex.w = 1.f; //as this is postional data ensure that w component is set to 1
							vertexData.push_back(vertex);
						}
						if (key == "vn")
						{
							vec4 normal = processVectorString(value);
							normal.w = 0.f; // as this is directional data ensure that w component is set to 0
							normalData.push_back(normal);
						}
						if (key == "vt")
						{
							vec4 vec = processVectorString(value);
							vec2 uvCoord = { vec.x, vec.y };
						}
						if (key == "f")
						{
							std::vector<std::string> faceComponents = splitStringAtCharacter(value, ' ');
							std::vector<uint32_t>faceIndices;
							for (auto iter = faceComponents.begin(); iter != faceComponents.end(); ++iter)
							{
								//see if the face has already been processed
								auto searchKey = faceIndexMap.find(*iter);
								if (searchKey != faceIndexMap.end())
								{
									//we have already processed this vertex
									faceIndices.push_back((*searchKey).second);
								}
								else
								{
									OBJVertex vertex = processFaceData(*iter, vertexData, normalData, textureData);
									meshData.push_back(vertex);
									uint32_t index = ((uint32_t)meshData.size() - 1);
									faceIndexMap[*iter] = index;
									faceIndices.push_back(index);
								}
								//now that all triplets have been processed process indexbuffer
								for (int i = 1; i < faceIndices.size() - 1; i++)
								{
									meshIndices.push_back(faceIndices[0]);
									meshIndices.push_back(faceIndices[i]);
									meshIndices.push_back(faceIndices[(size_t)i + 1]);
								}

								
							}
						}
					}
				}
			}
			vertexData.clear();
			normalData.clear();
			textureData.clear();
			std::cout << "Processed " << meshData.size() << " vertices in OBJ Mesh Data" << std::endl;
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


