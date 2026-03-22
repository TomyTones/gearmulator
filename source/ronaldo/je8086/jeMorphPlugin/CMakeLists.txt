cmake_minimum_required(VERSION 3.15)

project(jeMorphPlugin VERSION ${CMAKE_PROJECT_VERSION})

set(JEPI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../jeJucePlugin")

# All shared jeJucePlugin sources — we include everything except jeCreatePlugin.cpp
# (which creates the original AudioPluginAudioProcessor). Our jeMorphProcessor.cpp
# provides its own createPluginFilter() returning MorphProcessor instead.
set(SOURCES
	jeMorphProcessor.cpp jeMorphProcessor.h
	jeMorphEditorState.cpp jeMorphEditorState.h

	${JEPI_DIR}/jeAssign.cpp         ${JEPI_DIR}/jeAssign.h
	${JEPI_DIR}/jeController.cpp     ${JEPI_DIR}/jeController.h
	${JEPI_DIR}/jeEditor.cpp         ${JEPI_DIR}/jeEditor.h
	${JEPI_DIR}/jeFocusedParameter.cpp ${JEPI_DIR}/jeFocusedParameter.h
	${JEPI_DIR}/jeLcd.cpp            ${JEPI_DIR}/jeLcd.h
	${JEPI_DIR}/jePartButton.cpp     ${JEPI_DIR}/jePartButton.h
	${JEPI_DIR}/jePartSelect.cpp     ${JEPI_DIR}/jePartSelect.h
	${JEPI_DIR}/jePatchManager.cpp   ${JEPI_DIR}/jePatchManager.h
	${JEPI_DIR}/jePluginEditorState.cpp ${JEPI_DIR}/jePluginEditorState.h
	${JEPI_DIR}/jePluginProcessor.cpp   ${JEPI_DIR}/jePluginProcessor.h
	${JEPI_DIR}/jeTypes.h
	${JEPI_DIR}/parameterDescriptions_je.json
)

# Embed parameter descriptions and the jeTrancy skin from jeJucePlugin.
SET(ASSETS "${JEPI_DIR}/parameterDescriptions_je.json")

addSkin("JE8086Morph" "jeTrancy" "../jeJucePlugin/skins/jeTrancy" "jeTrancy.rml")
buildSkinHeader()

juce_add_binary_data(jeMorphPlugin_BinaryData SOURCES ${ASSETS} ${ASSETS_SKINS})

createJucePlugin(jeMorphPlugin "JE8086Morph" TRUE "Tjpm" jeMorphPlugin_BinaryData jeLib)

# Expose jeJucePlugin headers so jeMorphProcessor.h can #include "jePluginProcessor.h"
target_include_directories(jeMorphPlugin PRIVATE ${JEPI_DIR})
