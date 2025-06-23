#ifndef SCENELOADER_H
#define SCENELOADER_H

#include <string>

namespace Ogre {

class SceneManager;
class SceneNode;

} // namespace Ogre

void loadSceneWithAssimp(const std::string& filename, Ogre::SceneManager* sceneMgr, Ogre::SceneNode* parentNode);

#endif // SCENELOADER_H
