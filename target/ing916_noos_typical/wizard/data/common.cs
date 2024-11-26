echo|update start file
{{if keil}}rename|{{proj_dir}}/src/startup_{{old.family}}00.s|{{proj_dir}}/src/startup_{{family}}00.s{{endif}}
{{if iar}}rename|{{proj_dir}}/src/cstartup_{{family}}00.s|{{proj_dir}}/src/cstartup_{{family}}00.s{{endif}}
{{if ide in gcc segger}}rename|{{proj_dir}}/src/gstartup_{{family}}00.s|{{proj_dir}}/src/gstartup_{{family}}00.s{{endif}}
{{if rowley}}rename|{{proj_dir}}/src/cwstartup_{{family}}00.s|{{proj_dir}}/src/cwstartup_{{family}}00.s{{endif}}

echo|fixing {{proj_dir}}/flash_download.ini
change_ini|{{proj_dir}}/flash_download.ini|main|family|{{family}}
{{if secondary}}{{else}}change_ini|{{proj_dir}}/flash_download.ini|bin-0|FileName|{{sdk_release_path}}/bundles/{{bundle}}/{{series}}/platform.bin{{endif}}
{{if secondary}}{{else}}change_ini|{{proj_dir}}/flash_download.ini|bin-0|Address|{{bundle_meta.rom.base}}{{endif}}


