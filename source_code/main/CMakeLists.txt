# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

set(SOURCES
    "main.c"
    "lib/ini.c"
    "config.c"
    "barcos/barco.c"
    "canal/canal.c"
    "canal/flow_policy.c"
    "scheduler/scheduler_factory.c"
    "scheduler/scheduler_fcfs.c"
    "scheduler/scheduler_sjf.c"
    "scheduler/scheduler_prio.c"
    "scheduler/scheduler_strn.c"
    "scheduler/scheduler_rr.c"
    "input/input_task.c"

    
    )

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS "."        # optional, add here public include directories
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES            # optional, list the public requirements (component names)
    PRIV_REQUIRES       # optional, list the private requirements
    EMBED_FILES "config.ini"
    
)
