#include "moduleconfig.h"
#include "json.hpp"

ModuleConfig::ModuleConfig()
{

}

void ModuleConfig::Load(void)
{
    // write prettified JSON to another file
    //std::ofstream o("pretty.json");
    //o << std::setw(4) << j2 << std::endl;

    // read a JSON file
    std::ifstream i("config1.json");
    json j;

    std::cout << j["pi"] << j["pi2"].is_null();
    //printf("\nOk Done!\n ");
    //_getch();
}
