save|old_keil_family|{{if old.family eq ing918}}ING91800{{endif}}{{if old.family eq ing916}}ING91600{{endif}}
save|new_keil_family|{{if family eq ing918}}ING91800{{endif}}{{if family eq ing916}}ING91600{{endif}}
{{if family eq ing918}}save|cpu_str|CLOCK(32000000) CPUTYPE("Cortex-M3") ELITTLE IROM(0x4000-0x43fff) IRAM(0x20000000-0x2000ffff){{endif}}
{{if family eq ing916}}save|cpu_str|CLOCK(128000000) CPUTYPE("Cortex-M4") FPU2 ELITTLE IROM(0x2000000-0x21fffff) IRAM(0x20000000-0x20007fff){{endif}}

echo|update {{proj_dir}}/{{proj_name}}.uvproj
replace_all|{{proj_dir}}/{{proj_name}}.uvproj|{{old.series}}|{{series}}
replace_all|{{proj_dir}}/{{proj_name}}.uvproj|{{old.family}}|{{family}}
replace_all|{{proj_dir}}/{{proj_name}}.uvproj|{{old_keil_family}}|{{new_keil_family}}
change_xml|{{proj_dir}}/{{proj_name}}.uvproj|Targets>Target>TargetOption>TargetCommonOption>Cpu|{{cpu_str}}

echo|update {{proj_dir}}/{{proj_name}}.uvprojx
replace_all|{{proj_dir}}/{{proj_name}}.uvprojx|{{old.series}}|{{series}}
replace_all|{{proj_dir}}/{{proj_name}}.uvprojx|{{old.family}}|{{family}}
replace_all|{{proj_dir}}/{{proj_name}}.uvprojx|{{old_keil_family}}|{{new_keil_family}}
change_xml|{{proj_dir}}/{{proj_name}}.uvprojx|Targets>Target>TargetOption>TargetCommonOption>Cpu|{{cpu_str}}

remove|old_keil_family
remove|new_keil_family
remove|cpu_str
