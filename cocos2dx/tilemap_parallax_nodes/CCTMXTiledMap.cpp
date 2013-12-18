/****************************************************************************
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2009-2010 Ricardo Quesada
Copyright (c) 2011      Zynga Inc.

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/
#include "CCTMXTiledMap.h"
#include "CCTMXXMLParser.h"
#include "CCTMXLayer.h"
#include "sprite_nodes/CCSprite.h"

NS_CC_BEGIN

// implementation TMXTiledMap

TMXTiledMap * TMXTiledMap::create(const char *tmxFile)
{
    TMXTiledMap *pRet = new TMXTiledMap();
    if (pRet->initWithTMXFile(tmxFile))
    {
        pRet->autorelease();
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    return NULL;
}

TMXTiledMap* TMXTiledMap::createWithXML(const char* tmxString, const char* resourcePath)
{
    TMXTiledMap *pRet = new TMXTiledMap();
    if (pRet->initWithXML(tmxString, resourcePath))
    {
        pRet->autorelease();
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    return NULL;
}

bool TMXTiledMap::initWithTMXFile(const char *tmxFile)
{
    CCASSERT(tmxFile != NULL && strlen(tmxFile)>0, "TMXTiledMap: tmx file should not bi NULL");
    
    setContentSize(Size::ZERO);

    TMXMapInfo *mapInfo = TMXMapInfo::create(tmxFile);

    if (! mapInfo)
    {
        return false;
    }
    CCASSERT( mapInfo->getTilesets()->count() != 0, "TMXTiledMap: Map not found. Please check the filename.");
    buildWithMapInfo(mapInfo);

    return true;
}

bool TMXTiledMap::initWithXML(const char* tmxString, const char* resourcePath)
{
    setContentSize(Size::ZERO);

    TMXMapInfo *mapInfo = TMXMapInfo::createWithXML(tmxString, resourcePath);

    CCASSERT( mapInfo->getTilesets()->count() != 0, "TMXTiledMap: Map not found. Please check the filename.");
    buildWithMapInfo(mapInfo);

    return true;
}

TMXTiledMap::TMXTiledMap()
    :_mapSize(Size::ZERO)
    ,_tileSize(Size::ZERO)        
    ,_objectGroups(NULL)
    ,_properties(NULL)
    ,_tileProperties(NULL)
{
}
TMXTiledMap::~TMXTiledMap()
{
    CC_SAFE_RELEASE(_properties);
    CC_SAFE_RELEASE(_objectGroups);
    CC_SAFE_RELEASE(_tileProperties);
}

// private
TMXLayer * TMXTiledMap::parseLayer(TMXLayerInfo *layerInfo, TMXMapInfo *mapInfo)
{
    TMXTilesetInfo *tileset = tilesetForLayer(layerInfo, mapInfo);
    TMXLayer *layer = TMXLayer::create(tileset, layerInfo, mapInfo);

    // tell the layerinfo to release the ownership of the tiles map.
    layerInfo->_ownTiles = false;
    layer->setupTiles();

    return layer;
}

TMXTilesetInfo * TMXTiledMap::tilesetForLayer(TMXLayerInfo *layerInfo, TMXMapInfo *mapInfo)
{
    Size size = layerInfo->_layerSize;
    Array* tilesets = mapInfo->getTilesets();
    if (tilesets && tilesets->count()>0)
    {
        TMXTilesetInfo* tileset = NULL;
        Object* pObj = NULL;
        CCARRAY_FOREACH_REVERSE(tilesets, pObj)
        {
            tileset = static_cast<TMXTilesetInfo*>(pObj);
            if (tileset)
            {
                for( unsigned int y=0; y < size.height; y++ )
                {
                    for( unsigned int x=0; x < size.width; x++ ) 
                    {
                        unsigned int pos = (unsigned int)(x + size.width * y);
                        unsigned int gid = layerInfo->_tiles[ pos ];

                        // gid are stored in little endian.
                        // if host is big endian, then swap
                        //if( o == CFByteOrderBigEndian )
                        //    gid = CFSwapInt32( gid );
                        /* We support little endian.*/

                        // XXX: gid == 0 --> empty tile
                        if( gid != 0 ) 
                        {
                            // Optimization: quick return
                            // if the layer is invalid (more than 1 tileset per layer) an CCAssert will be thrown later
                            if( (gid & kFlippedMask) >= tileset->_firstGid )
                                return tileset;
                        }
                    }
                }        
            }
        }
    }

    // If all the tiles are 0, return empty tileset
    CCLOG("cocos2d: Warning: TMX Layer '%s' has no tiles", layerInfo->_name.c_str());
    return NULL;
}

//lw begin
void lwSplitLayerInfo(std::vector<TMXLayerInfo*>& out, TMXLayerInfo* in, TMXMapInfo* mapInfo) {
    Array* tilesets = mapInfo->getTilesets();
    std::vector<int> firstIds;
    std::vector<unsigned int*> layersTiles;
    std::vector<unsigned int> mins;
    std::vector<unsigned int> maxs;
    
    TMXTilesetInfo* tileset = nullptr;
    Object* pObj = nullptr;
    CCARRAY_FOREACH_REVERSE(tilesets, pObj){
        tileset = static_cast<TMXTilesetInfo*>(pObj);
        firstIds.push_back(tileset->_firstGid);
        layersTiles.push_back(NULL);
        mins.push_back(10000);
        maxs.push_back(0);
    }
    
    int idxMax = in->_layerSize.width * in->_layerSize.height;
    for (int i = 0; i < idxMax; ++i) {
        unsigned int gid = in->_tiles[i];
        if (gid != 0) {
            size_t n = firstIds.size();
            for (size_t j = 0; j < n ; ++j) {
                if (gid >= firstIds[j]) {
                    if (layersTiles[j] == NULL) {
                        layersTiles[j] = new unsigned int [idxMax];
                        memset(layersTiles[j], 0, idxMax*sizeof(unsigned int));
                    }
                    layersTiles[j][i] = gid;
                    mins[j] = MIN(gid, mins[j]);
                    maxs[j] = MAX(gid, maxs[j]);
                    break;
                }
            }
        }
    }
    
    int i = 0;
    for (auto it = layersTiles.begin(); it != layersTiles.end(); ++it, ++i) {
        if (*it) {
            TMXLayerInfo *layerInfo = new TMXLayerInfo;
            layerInfo->autorelease();
            layerInfo->setProperties(in->getProperties());
            layerInfo->_name = in->_name;
            layerInfo->_layerSize = in->_layerSize;
            layerInfo->_tiles = (*it);
            layerInfo->_visible = in->_visible;
            layerInfo->_opacity = in->_opacity;
            layerInfo->_ownTiles = in->_ownTiles;
            layerInfo->_minGID = mins[i];
            layerInfo->_maxGID = maxs[i];
            layerInfo->_offset = in->_offset;
            out.push_back(layerInfo);
        }
    }
}
//lw end

void TMXTiledMap::buildWithMapInfo(TMXMapInfo* mapInfo)
{
    _mapSize = mapInfo->getMapSize();
    _tileSize = mapInfo->getTileSize();
    _mapOrientation = mapInfo->getOrientation();

    CC_SAFE_RELEASE(_objectGroups);
    _objectGroups = mapInfo->getObjectGroups();
    CC_SAFE_RETAIN(_objectGroups);

    CC_SAFE_RELEASE(_properties);
    _properties = mapInfo->getProperties();
    CC_SAFE_RETAIN(_properties);

    CC_SAFE_RELEASE(_tileProperties);
    _tileProperties = mapInfo->getTileProperties();
    CC_SAFE_RETAIN(_tileProperties);

    int idx=0;

    Array* layers = mapInfo->getLayers();
    if (layers && layers->count()>0)
    {
        TMXLayerInfo* layerInfo = NULL;
        Object* pObj = NULL;
        CCARRAY_FOREACH(layers, pObj)
        {
            layerInfo = static_cast<TMXLayerInfo*>(pObj);
            
            //lwadd begin
            std::vector<TMXLayerInfo*> layerInfos;
            lwSplitLayerInfo(layerInfos, layerInfo, mapInfo);
            
            for (auto it = layerInfos.begin(); it != layerInfos.end(); ++it) {
                if ((*it)->_visible) {
                    TMXLayer *child = parseLayer(*it, mapInfo);
                    addChild((Node*)child, idx, idx);
                    
                    // update content size with the max size
                    const Size& childSize = child->getContentSize();
                    Size currentSize = this->getContentSize();
                    currentSize.width = MAX( currentSize.width, childSize.width );
                    currentSize.height = MAX( currentSize.height, childSize.height );
                    this->setContentSize(currentSize);
                    
                    idx++;
                }
            }
            //lwadd end
            
            //lwdel begin
//            if (layerInfo && layerInfo->_visible)
//            {
//                TMXLayer *child = parseLayer(layerInfo, mapInfo);
//                addChild((Node*)child, idx, idx);
//
//                // update content size with the max size
//                const Size& childSize = child->getContentSize();
//                Size currentSize = this->getContentSize();
//                currentSize.width = MAX( currentSize.width, childSize.width );
//                currentSize.height = MAX( currentSize.height, childSize.height );
//                this->setContentSize(currentSize);
//
//                idx++;
//            }
            //lwdel end
        }
    }
}

// public
TMXLayer * TMXTiledMap::getLayer(const char *layerName) const
{
    CCASSERT(layerName != NULL && strlen(layerName) > 0, "Invalid layer name!");
    Object* pObj = NULL;
    CCARRAY_FOREACH(_children, pObj) 
    {
        TMXLayer* layer = dynamic_cast<TMXLayer*>(pObj);
        if(layer)
        {
            if(0 == strcmp(layer->getLayerName(), layerName))
            {
                return layer;
            }
        }
    }

    // layer not found
    return NULL;
}

TMXObjectGroup * TMXTiledMap::getObjectGroup(const char *groupName) const
{
    CCASSERT(groupName != NULL && strlen(groupName) > 0, "Invalid group name!");

    std::string sGroupName = groupName;
    if (_objectGroups && _objectGroups->count()>0)
    {
        TMXObjectGroup* objectGroup = NULL;
        Object* pObj = NULL;
        CCARRAY_FOREACH(_objectGroups, pObj)
        {
            objectGroup = static_cast<TMXObjectGroup*>(pObj);
            if (objectGroup && objectGroup->getGroupName() == sGroupName)
            {
                return objectGroup;
            }
        }
    }

    // objectGroup not found
    return NULL;
}

String* TMXTiledMap::getProperty(const char *propertyName) const
{
    return static_cast<String*>(_properties->objectForKey(propertyName));
}

Dictionary* TMXTiledMap::getPropertiesForGID(int GID) const
{
    return static_cast<Dictionary*>(_tileProperties->objectForKey(GID));
}
        

NS_CC_END

