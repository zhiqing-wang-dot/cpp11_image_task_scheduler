#include "image_task_scheduler/config.hpp"
#include "image_task_scheduler/dataset_pipline.hpp"

int main(int argc, char** argv)
{
    Config config;

    if (!config.parse(argc, argv, &config))
    {
        return 1;
    }

    DatasetPipline pipeline(config);

    if (!pipeline.run())
    {
        return 1;
    }

    return 0;
}
