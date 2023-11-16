#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <libxml/parser>

int main() {
    // Open the XML file for reading
    std::ifstream xmlFile("Master.xml");

    if (!xmlFile.is_open()) {
        std::cerr << "Error opening XML file." << std::endl;
        return 1;
    }

    // Read the XML content into a string
    std::string xmlContent((std::istreambuf_iterator<char>(xmlFile)), std::istreambuf_iterator<char>());

    // Parse the XML content using RapidXML
    rapidxml::xml_document<> doc;
    doc.parse<0>(&xmlContent[0]);

    // Create a CSV file for writing
    std::ofstream csvFile("Master.csv");

    if (!csvFile.is_open()) {
        std::cerr << "Error opening CSV file." << std::endl;
        return 1;
    }

    // Iterate through the XML nodes and extract the data
    for (rapidxml::xml_node<>* groupNode = doc.first_node("TALLYMESSAGE")->first_node("GROUP");
         groupNode; groupNode = groupNode->next_sibling("GROUP")) {

        // Extract the relevant data from the XML
        std::string groupName = groupNode->first_attribute("NAME")->value();
        std::string parent = groupNode->first_node("PARENT")->value();
        std::string isBillwiseOn = groupNode->first_node("ISBILLWISEON")->value();
        std::string isSubledger = groupNode->first_node("ISSUBLEDGER")->value();

        // Write the data to the CSV file
        csvFile << groupName << "," << parent << "," << isBillwiseOn << "," << isSubledger << std::endl;
    }

    // Close the CSV file
    csvFile.close();

    std::cout << "Conversion completed successfully." << std::endl;
    return 0;
}

