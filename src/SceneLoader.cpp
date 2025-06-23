#include <OgreRoot.h>
#include <OgreSceneManager.h>
#include <OgreItem.h>
#include <OgreMeshManager2.h>
#include <OgreMesh2.h>
#include <OgreSubMesh2.h>
// #include <OgreRenderOperation.h>
#include <OgreHlmsManager.h>
#include <OgreHlmsPbsDatablock.h>
#include <OgreHlmsPbs.h>
#include <Vao/OgreVaoManager.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <string>

static void processAssimpLights(const aiScene* scene, Ogre::SceneManager* sceneMgr)
{
    std::cout << "LIGHTS:" << std::endl;
    for (unsigned int i = 0; i < scene->mNumLights; ++i)
    {
        const aiLight* aiLight = scene->mLights[i];
        std::cout << "\tLIGHT " << i << " aiLight->mType = " << aiLight->mType << std::endl;
        Ogre::String lightName = aiLight->mName.C_Str();

        Ogre::Light* light = sceneMgr->createLight();
        light->setName(lightName);

        switch (aiLight->mType)
        {
        case aiLightSource_DIRECTIONAL:
            light->setType(Ogre::Light::LT_DIRECTIONAL);
            break;
        case aiLightSource_POINT:
            light->setType(Ogre::Light::LT_POINT);
            // light->setAttenuationBasedOnRadius(300, 0.001); // HACK just trying stuff...
            light->setPowerScale(0.002);
            break;
        case aiLightSource_SPOT:
            light->setType(Ogre::Light::LT_SPOTLIGHT);
            light->setSpotlightRange(
                Ogre::Radian(aiLight->mAngleInnerCone),
                Ogre::Radian(aiLight->mAngleOuterCone));
            break;
        default:
            continue; // Skip unsupported types
        }

        light->setDiffuseColour(aiLight->mColorDiffuse.r, aiLight->mColorDiffuse.g, aiLight->mColorDiffuse.b);
        light->setSpecularColour(aiLight->mColorSpecular.r, aiLight->mColorSpecular.g, aiLight->mColorSpecular.b);

        light->setAttenuation(
            100.0f,  // assume max range
            aiLight->mAttenuationConstant,
            aiLight->mAttenuationLinear,
            aiLight->mAttenuationQuadratic);

        Ogre::SceneNode* lightNode = sceneMgr->getRootSceneNode()->createChildSceneNode();

        if (aiLight->mType != aiLightSource_DIRECTIONAL)
            lightNode->setPosition(aiLight->mPosition.x, aiLight->mPosition.y, aiLight->mPosition.z);

        if (aiLight->mType != aiLightSource_POINT) {
            Ogre::Vector3 dir(aiLight->mDirection.x, aiLight->mDirection.y, aiLight->mDirection.z);
            lightNode->setDirection(dir.normalisedCopy());
        }

        lightNode->attachObject(light);
    }
}

static void processAssimpNode(const aiNode* node, const aiScene* scene, Ogre::SceneManager* sceneMgr, Ogre::SceneNode* parentNode)
{
    Ogre::SceneNode* currentNode = parentNode->createChildSceneNode();

    aiMatrix4x4 transform = node->mTransformation;
    aiVector3t<float> scaling, position;
    aiQuaterniont<float> rotation;
    transform.Decompose(scaling, rotation, position);

    Ogre::Vector3 scale(scaling.x, scaling.y, scaling.z);
    Ogre::Vector3 translate(position.x, position.y, position.z);
    Ogre::Quaternion orient(rotation.w, rotation.x, rotation.y, rotation.z);

    currentNode->setPosition(translate);
    currentNode->setOrientation(orient);
    currentNode->setScale(scale);

    auto vaoManager = Ogre::Root::getSingleton().getRenderSystem()->getVaoManager();
    auto meshMgr = Ogre::MeshManager::getSingletonPtr();

    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        const aiMesh * const aiMesh = scene->mMeshes[node->mMeshes[i]];
        const Ogre::String meshName = "AssimpMesh_" + Ogre::StringConverter::toString(node->mMeshes[i]);

        // std::cout << "LOADING MESH #" << m <<

        size_t numVerts = aiMesh->mNumVertices;
        size_t numIndices = aiMesh->mNumFaces * 3;

        std::vector<float> vertexBuffer;
        std::vector<uint16_t> indexBuffer;

        Ogre::Vector3 minBounds(std::numeric_limits<float>::max());
        Ogre::Vector3 maxBounds(std::numeric_limits<float>::lowest());

        vertexBuffer.reserve(numVerts * 6);
        for (unsigned int i = 0; i < aiMesh->mNumVertices; ++i) {
            vertexBuffer.push_back(aiMesh->mVertices[i].x);
            vertexBuffer.push_back(aiMesh->mVertices[i].y);
            vertexBuffer.push_back(aiMesh->mVertices[i].z);

            {
                Ogre::Vector3 v(aiMesh->mVertices[i].x, aiMesh->mVertices[i].y, aiMesh->mVertices[i].z);
                minBounds.makeFloor(v);
                maxBounds.makeCeil(v);
            }

            if (aiMesh->HasNormals()) {
                vertexBuffer.push_back(aiMesh->mNormals[i].x);
                vertexBuffer.push_back(aiMesh->mNormals[i].y);
                vertexBuffer.push_back(aiMesh->mNormals[i].z);
            } else {
                vertexBuffer.push_back(0.0f);
                vertexBuffer.push_back(1.0f);
                vertexBuffer.push_back(0.0f);
            }
        }

        for (unsigned int i = 0; i < aiMesh->mNumFaces; ++i) {
            const aiFace &face = aiMesh->mFaces[i];
            for (unsigned int j = 0; j < 3; ++j) {
                indexBuffer.push_back(static_cast<uint16_t>(face.mIndices[j]));
            }
        }

        Ogre::VertexElement2Vec vertexElements;
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));

        Ogre::VertexBufferPacked *vb = vaoManager->createVertexBuffer(
            vertexElements, numVerts, Ogre::BT_DEFAULT, vertexBuffer.data(), false);

        Ogre::IndexBufferPacked *ib = vaoManager->createIndexBuffer(
            Ogre::IndexBufferPacked::IT_16BIT, numIndices, Ogre::BT_DEFAULT, indexBuffer.data(), false);

        Ogre::VertexArrayObject *vao = vaoManager->createVertexArrayObject({vb}, ib, Ogre::OT_TRIANGLE_LIST);

        // std::string meshName = "ImportedMesh_" + std::to_string(m);
        Ogre::MeshPtr mesh = meshMgr->createManual(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        Ogre::SubMesh *subMesh = mesh->createSubMesh();
        //subMesh->operationType = Ogre::OT_TRIANGLE_LIST;
        subMesh->mVao[Ogre::VpNormal].push_back(vao);

        {
            Ogre::Vector3 center = (minBounds + maxBounds) * 0.5f;
            Ogre::Vector3 halfSize = (maxBounds - minBounds) * 0.5f;
            Ogre::Aabb bounds(center, halfSize);
            mesh->_setBounds(bounds, false);
        }
        mesh->load();

        Ogre::Item *item = sceneMgr->createItem(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::SCENE_DYNAMIC);
        Ogre::SceneNode *node = sceneMgr->getRootSceneNode(Ogre::SCENE_DYNAMIC)->createChildSceneNode();
        // node->attachObject(item);
        
        
        
        aiMaterial * const aiMat = scene->mMaterials[aiMesh->mMaterialIndex];
        aiColor4D diffuseColor;
        if (AI_SUCCESS != aiGetMaterialColor(aiMat, AI_MATKEY_BASE_COLOR, &diffuseColor))
        {
            // Fall back to diffuse if base color isn't set
            aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_DIFFUSE, &diffuseColor);
        }

        
        
        const Ogre::String datablockName = meshName + "/Material";
        Ogre::HlmsDatablock *datablock = Ogre::Root::getSingleton().getHlmsManager()->getHlms(Ogre::HLMS_PBS)->createDatablock(
            datablockName, datablockName,
            Ogre::HlmsMacroblock(), Ogre::HlmsBlendblock(), Ogre::HlmsParamVec());

        Ogre::HlmsPbsDatablock * const pbs = static_cast<Ogre::HlmsPbsDatablock *>(datablock);
        pbs->setDiffuse(Ogre::Vector3(diffuseColor.r, diffuseColor.g, diffuseColor.b));

        item->setDatablock(pbs);


        currentNode->attachObject(item);
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        processAssimpNode(node->mChildren[i], scene, sceneMgr, currentNode);
    }
}

void loadSceneWithAssimp(const std::string& filename, Ogre::SceneManager* sceneMgr, Ogre::SceneNode* parentNode)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filename,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_SortByPType |
        aiProcess_PreTransformVertices
    );

    if (!scene || !scene->mRootNode) {
        std::cerr << "Error loading scene: " << importer.GetErrorString() << std::endl;
        return;
    }

    processAssimpNode(scene->mRootNode, scene, sceneMgr, parentNode);
    processAssimpLights(scene, sceneMgr);
}
