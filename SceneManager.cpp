///////////////////////////////////////////////////////////////////////////////
// SceneManager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
// 
// *** Modified by Connie Knupp on 10/06/2024   ***
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"
#include "ViewManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.diffuseColor = glm::vec3(0.41f, 0.41f, 0.41f);
	metalMaterial.specularColor = glm::vec3(0.502f, 0.502f, 0.502f);
	metalMaterial.shininess = 22.0;
	metalMaterial.tag = "metal";
	m_objectMaterials.push_back(metalMaterial);

	OBJECT_MATERIAL pumpkinMaterial;
	pumpkinMaterial.diffuseColor = glm::vec3(1.0f, 0.65f, 0.0f);
	pumpkinMaterial.specularColor = glm::vec3(1.0f, 0.85f, 0.0f);
	pumpkinMaterial.shininess = 12.0;
	pumpkinMaterial.tag = "pumpkin";
	m_objectMaterials.push_back(pumpkinMaterial);


	OBJECT_MATERIAL potionMaterial;
	potionMaterial.diffuseColor = glm::vec3(0.0f, 0.89f, 0.0f);
	potionMaterial.specularColor = glm::vec3(0.0f, 1.0f, 0.0f);
	potionMaterial.shininess = 15.0;
	potionMaterial.tag = "potion";
	m_objectMaterials.push_back(potionMaterial);

	OBJECT_MATERIAL strawMaterial;
	strawMaterial.diffuseColor = glm::vec3(0.10f, 0.089f, 0.071f);
	strawMaterial.specularColor = glm::vec3(0.10f, 0.089f, 0.0f);
	strawMaterial.shininess = 2.0;
	strawMaterial.tag = "straw";
	m_objectMaterials.push_back(strawMaterial);

	OBJECT_MATERIAL clothMaterial;
	clothMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	clothMaterial.specularColor = glm::vec3(0.15f, 0.15f, 0.15f);
	clothMaterial.shininess = 2.0;
	clothMaterial.tag = "cloth";
	m_objectMaterials.push_back(clothMaterial);

	OBJECT_MATERIAL cementMaterial;
	cementMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	cementMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	cementMaterial.shininess = 0.5;
	cementMaterial.tag = "cement";
	m_objectMaterials.push_back(cementMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.82f, 0.71f, 0.55f);
	woodMaterial.specularColor = glm::vec3(0.96f, 0.87f, 0.70f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL stemMaterial;
	stemMaterial.diffuseColor = glm::vec3(0.55f, 0.27f, 0.075f);
	stemMaterial.specularColor = glm::vec3(0.55f, 0.27f, 0.075f);
	stemMaterial.shininess = 0.2;
	stemMaterial.tag = "stem";
	m_objectMaterials.push_back(stemMaterial);

 }


/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** Connie Knupp added the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene.  Code from OpenGLSample used ***/
	/*** as a template.   ***/

	bool bReturn = false;

	bReturn = CreateGLTexture(
		"../textures/bat_face.jpg",
		"bat_face");

	bReturn = CreateGLTexture(
		"../textures/black_brim.jpg",
		"black_brim");

	bReturn = CreateGLTexture(
		"../textures/treebark1.jpg",
		"stem");

	bReturn = CreateGLTexture(
		"../textures/black_fur.jpg",
		"black_fur");

	bReturn = CreateGLTexture(
		"../textures/cauldron3.jpg",
		"cauldron");

	bReturn = CreateGLTexture(
		"../textures/pumpkin2.jpg",
		"pumpkin");

	bReturn = CreateGLTexture(
		"../textures/straw1.jpg",
		"straw_ends");

	bReturn = CreateGLTexture(
		"../textures/potion.jpg",
		"potion");

	bReturn = CreateGLTexture(
		"../textures/pavers.jpg",
		"pavers");

	bReturn = CreateGLTexture(
		"../textures/wood_planks.jpg",
		"wood_planks");


	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}


/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			//commented out to work with Apporto fragmentShader
			//m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			//m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the texture image files for the textures applied
	// to objects in the 3D scene
	LoadSceneTextures();

	// define the materials that will be used for the objects
	// in the 3D scene
	DefineObjectMaterials();

	// add and defile the light sources for the 3D scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/

void SceneManager::RenderScene()
{
	RenderBackground();
	RenderCauldron();
	RenderStrawBale();
	RenderFirstPumpkin();
	RenderSecondPumpkin();
	RenderWitchHat();
	RenderBat();

}

/***********************************************************
 *  RenderBackground()
 *
 *  This method is used for rendering the floor and backdrop
 * for the scene.
 ***********************************************************/

void SceneManager::RenderBackground()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh ***/
	/*** for the floor.*** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture for the basic mesh
	SetShaderTexture("pavers");
	SetTextureUVScale(1.0, 1.0);

	//set the material
	SetShaderMaterial("cement");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/***Set needed transformations before drawing the basic mesh for background.*** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 100.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 10.0f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture for the basic mesh
	SetShaderTexture("wood_planks");
	SetTextureUVScale(1.0, 1.0);

	//set the material
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
}

/***********************************************************
 *  RenderCauldron()
 *
 *  This method is used for rendering the CAULDRON
 ***********************************************************/
void SceneManager::RenderCauldron()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	/*** Set needed transformations before drawing the half-sphere ***/
	/*** for the base of the cauldron.*** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.5f, 2.5f, 2.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.5f, 2.88F, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	//Set the texture for the basic mesh
	SetShaderTexture("cauldron");
	SetTextureUVScale(1.0, 1.0);

	//set the material
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfSphereMesh();

	/*** Set needed transformations before drawing the half-sphere ***/
	/*** for the base of the cauldron.*** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.5f, 0.5f, 2.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.5f, 2.88F, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	//Set the texture for the basic mesh
	SetShaderTexture("potion");
	SetTextureUVScale(1.0, 1.0);

	//set the material
	SetShaderMaterial("potion");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	/**************************************************************/
	/*** Set needed transformations before drawing the torus ***/
	/*** for the RIM of the cauldron.*** /
	/**************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.2f, 2.2f, 2.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.5f, 2.88f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	//Set the texture for the basic mesh
	SetShaderTexture("cauldron");

	//set the material
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/***********************************************************/
	/*** Set needed transformations before drawing the first ***/
	/*** leg for the cauldron.*** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.35f, 0.9f, 0.35f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = 20.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.8f, 0.9f, 0.25f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture for the basic mesh
	SetShaderTexture("cauldron");

	//set the material
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	/************************************************************/
	/*** Set needed transformations before drawing the second ***/
	/*** leg for the cauldron.*** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.35f, 0.9f, 0.35f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 110.0f;
	ZrotationDegrees = 20.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.5f, 0.9f, -1.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture for the basic mesh
	SetShaderTexture("cauldron");

	//set the material
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	/************************************************************/
	/*** Set needed transformations before drawing the third ***/
	/*** leg for the cauldron.*** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.35f, 0.9f, 0.35f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 230.0f;
	ZrotationDegrees = 20.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.5f, 0.9f, 1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture for the basic mesh
	SetShaderTexture("cauldron");

	//set the material
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();
}

/***********************************************************
 *  RenderStrawBale()
 *
 *  This method is used for rendering the CAULDRON
 ***********************************************************/
void SceneManager::RenderStrawBale()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	/*** Set needed transformations before drawing the half-sphere ***/
	/*** for the base of the cauldron.*** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.0, 8.0f, 4.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 125.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.5f, 2.001F, -1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture for the basic mesh
	SetShaderTexture("straw_ends");
	SetTextureUVScale(1.0, 1.0);

	//set the material
	SetShaderMaterial("straw");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
}
/***********************************************************
 *  RenderFirstPumpkin()
 *
 *  This method is used for rendering the first pumpkin
 ***********************************************************/
void SceneManager::RenderFirstPumpkin()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	/*** Set needed transformations before drawing the sphere ***/
	/*** for the base of the pumpkin.*** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.6, 1.4f, 1.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.5f, 5.251F, 1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture for the basic mesh
	SetShaderTexture("pumpkin");
	SetTextureUVScale(1.0, 1.0);

	//set the material
	SetShaderMaterial("pumpkin");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	/*** Set needed transformations before drawing the sphere ***/
	/*** for the stem of the pumpkin.*** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3, 0.6f, 0.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 15.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.5f, 6.551F, 1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture for the basic mesh
	SetShaderTexture("stem");
	SetTextureUVScale(1.0, 1.0);

	//set the material
	SetShaderMaterial("stem");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

}

/***********************************************************
 *  RenderSecondPumpkin()
 *
 *  This method is used for rendering the second pumpkin
 ***********************************************************/
void SceneManager::RenderSecondPumpkin()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	/*** Set needed transformations before drawing the sphere ***/
	/*** for the base of the pumpkin.*** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.9, 1.5f, 1.9f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.25f, 5.451F, -2.1);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture for the basic mesh
	SetShaderTexture("pumpkin");
	SetTextureUVScale(1.0, 1.0);

	//set the material
	SetShaderMaterial("pumpkin");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
}

/***********************************************************
 *  RenderWitchHat()
 *
 *  This method is used for rendering the witch hat
 ***********************************************************/
void SceneManager::RenderWitchHat()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	/*** Set needed transformations before drawing  ***/
	/*** the cone of the hat *** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2, 3.7f, 1.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = -7.5f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.2f, 6.8F, -2.3);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture for the basic mesh
	SetShaderTexture("black_brim");
	SetTextureUVScale(1.0, 1.0);

	//set the material
	SetShaderMaterial("cloth");

	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();

	/*** Set needed transformations before drawing  ***/
	/*** the brim of the hat *** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.25, 0.1f, 2.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = -7.5f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.2f, 6.8F, -2.3);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture for the basic mesh
	SetShaderTexture("black_brim");
	SetTextureUVScale(1.0, 1.0);

	//set the material
	SetShaderMaterial("cloth");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
}

/***********************************************************
 *  RenderBat()
 *
 *  This method is used for rendering the bat
 ***********************************************************/
void SceneManager::RenderBat() 
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	/*** Set needed transformations before drawing the sphere ***/
	/*** for the head of the bat.*** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.75, 0.5f, 0.62f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 20.0f;
	YrotationDegrees = 25.0f;
	ZrotationDegrees = 20.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3( -7.3f, 9.88F, -3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture for the basic mesh
	SetShaderTexture("bat_face");
	SetTextureUVScale(1.0, 1.0);

	//set the material
	SetShaderMaterial("cloth");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	/*** Set needed transformations before drawing the sphere ***/
	/*** for the body of the bat.*** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.25, 1.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 20.0f;
	YrotationDegrees = 20.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.0f, 9.2F, -3.95f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture for the basic mesh
	SetShaderTexture("black_fur");
	SetTextureUVScale(1.0, 1.0);

	//set the material
	SetShaderMaterial("cloth");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	
	/*** Set needed transformations before drawing the prism ***/
	/*** for the first part of the bat's left wing *** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.6f, 0.15f, 1.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 155.0f;
	YrotationDegrees = 25.0f;
	ZrotationDegrees = -30.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.7f, 9.9F, -3.85f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh();

	/*** Set needed transformations before drawing the prism ***/
	/*** for the second part of the bat's left wing *** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.9f, 0.15f, 1.9f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 155.0f;
	YrotationDegrees = 35.0f;
	ZrotationDegrees = -30.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.7f, 10.58F, -3.65f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh();
	
	/*** Set needed transformations before drawing the prism ***/
	/*** for the THIRD part of the bat's left wing *** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.1f, 0.15f, 1.9f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 155.0f;
	YrotationDegrees = 50.0f;
	ZrotationDegrees = -30.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.7f, 11.5f, -3.15f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh();

	/*** Set needed transformations before drawing the prism ***/
	/*** for the first part of the bat's right wing *** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.65f, 0.15f, 1.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 140.0f;
	YrotationDegrees = -23.0f;
	ZrotationDegrees = -25.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-8.65f, 8.6f, -3.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh();

	/*** Set needed transformations before drawing the prism ***/
	/*** for the second part of the bat's right wing *** /
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.9f, 0.15f, 1.9f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 140.0f;
	YrotationDegrees = -27.0f;
	ZrotationDegrees = -25.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-9.6f, 8.45f, -3.1f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh();

	/*** Set needed transformations before drawing the prism ***/
	/*** for the THIRD part of the bat's right wing *** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.1f, 0.15f, 1.9f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 140.0f;
	YrotationDegrees = -35.0f;
	ZrotationDegrees = -25.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-10.8f, 8.45f, -2.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh();

	/*** Set needed transformations before drawing the tapered ***/
	/*** cylinder for the bat's left leg *** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1, 0.5f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 55.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.8f, 8.4f, -4.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();


	/*** Set needed transformations before drawing the tapered ***/
	/*** cylinder for the bat's right leg *** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1, 0.5f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0;
	YrotationDegrees = 135.0f;
	ZrotationDegrees = 25.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.3f, 7.8f, -4.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	/*** Set needed transformations before drawing the ***/
	/*** Half Sphere for the bat's left ear *** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.28, 0.25f, 0.78f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -50.0;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 20.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.9f, 10.25f, -2.85f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfSphereMesh();

	/*** Set needed transformations before drawing the ***/
	/*** Half Sphere for the bat's right ear *** /
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.28, 0.25f, 0.78f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -25.0;
	YrotationDegrees = -60.0f;
	ZrotationDegrees = 60.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.8f, 9.9f, -2.55f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfSphereMesh();
}



/***********************************************************
*  SetupSceneLights()
*
*  This method is called to add and configure the light
*  sources for the 3D scene.  There are up to 4 light sources.
***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting - to use the default rendered 
	// lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);
	
	//directional lighting to emulate moonlight coming into the scene. 
	m_pShaderManager->setVec3Value("directionalLight.direction", -0.05f, -0.03f, -10.1f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.282f, 0.239f, 0.54f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.06f, 0.06f, 0.06f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	//point light 1
	m_pShaderManager->setVec3Value("pointLights[0].direction", 5.0, 14.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.0f, 0.003f, 0.0);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.01f, 0.05f, 0.01f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.1f, 0.3f, 0.1f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	//spotlight
	m_pShaderManager->setVec3Value("spotlight.ambient", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("spotlight.diffuse", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("spotlight.specular", 0.7f, 0.7f, 0.7f);
	m_pShaderManager->setFloatValue("spotlight.constant", 1.0f);
	m_pShaderManager->setFloatValue("spotlight.linear", 0.09f);
	m_pShaderManager->setFloatValue("spotlight.quadratic", 0.032f);
	m_pShaderManager->setFloatValue("spotlight.cutOff", glm::cos(glm::radians(42.5f)));
	m_pShaderManager->setFloatValue("spotlight.outerCutoff", glm::cos(glm::radians(48.0f)));
	m_pShaderManager->setBoolValue("spotlight.bActive", true);


}

