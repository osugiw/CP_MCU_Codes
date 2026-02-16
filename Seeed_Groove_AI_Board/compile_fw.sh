# Compiling the fW
DIR_SDK="EPII_CM55M_APP_S/" 

echo "****** Compiling firmware ******"
make clean -C $DIR_SDK
make -C $DIR_SDK

# Copying compiled file
DIR_IMAGE_GEN="we2_image_gen_local/"
SRC_ELF="${DIR_SDK}obj_epii_evb_icv30_bdv10/gnu_epii_evb_WLCSP65/EPII_CM55M_gnu_epii_evb_WLCSP65_s.elf"
DIR_FW_CASE_ONE="${DIR_IMAGE_GEN}input_case1_secboot/"
echo "****** Generating FW image ******"
cp "${SRC_ELF}" "${DIR_FW_CASE_ONE}"

# Generate .img
TOOL_GEN="we2_local_image_gen"
RULES_SETTINGS="project_case1_blp_wlcsp.json"
cd "${DIR_IMAGE_GEN}"
./"${TOOL_GEN}" "${RULES_SETTINGS}"
cd ..