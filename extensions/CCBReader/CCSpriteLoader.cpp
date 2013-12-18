#include "CCSpriteLoader.h"

#define PROPERTY_FLIP "flip"
#define PROPERTY_DISPLAYFRAME "displayFrame"
#define PROPERTY_COLOR "color"
#define PROPERTY_OPACITY "opacity"
#define PROPERTY_BLENDFUNC "blendFunc"

NS_CC_EXT_BEGIN

void SpriteLoader::onHandlePropTypeSpriteFrame(Node * pNode, Node * pParent, const char * pPropertyName, SpriteFrame * pSpriteFrame, CCBReader * ccbReader) {
    if(strcmp(pPropertyName, PROPERTY_DISPLAYFRAME) == 0) {
        if(pSpriteFrame != NULL) {
            ((Sprite *)pNode)->setDisplayFrame(pSpriteFrame);
        } else {
            CCLOG("ERROR: SpriteFrame NULL");
        }
    } else {
        NodeLoader::onHandlePropTypeSpriteFrame(pNode, pParent, pPropertyName, pSpriteFrame, ccbReader);
    }
}

void SpriteLoader::onHandlePropTypeFlip(Node * pNode, Node * pParent, const char * pPropertyName, bool * pFlip, CCBReader * ccbReader) {
    if(strcmp(pPropertyName, PROPERTY_FLIP) == 0) {
        ((Sprite *)pNode)->setFlippedX(pFlip[0]);
        ((Sprite *)pNode)->setFlippedY(pFlip[1]);
    } else {
        NodeLoader::onHandlePropTypeFlip(pNode, pParent, pPropertyName, pFlip, ccbReader);
    }
}

void SpriteLoader::onHandlePropTypeColor3(Node * pNode, Node * pParent, const char * pPropertyName, Color3B pColor3B, CCBReader * ccbReader) {
    if(strcmp(pPropertyName, PROPERTY_COLOR) == 0) {
        ((Sprite *)pNode)->setColor(pColor3B);
    } else {
        NodeLoader::onHandlePropTypeColor3(pNode, pParent, pPropertyName, pColor3B, ccbReader);
    }
}

void SpriteLoader::onHandlePropTypeByte(Node * pNode, Node * pParent, const char * pPropertyName, unsigned char pByte, CCBReader * ccbReader) {
    if(strcmp(pPropertyName, PROPERTY_OPACITY) == 0) {
        ((Sprite *)pNode)->setOpacity(pByte);
    } else {
        NodeLoader::onHandlePropTypeByte(pNode, pParent, pPropertyName, pByte, ccbReader);
    }
}

void SpriteLoader::onHandlePropTypeBlendFunc(Node * pNode, Node * pParent, const char * pPropertyName, BlendFunc pBlendFunc, CCBReader * ccbReader) {
    if(strcmp(pPropertyName, PROPERTY_BLENDFUNC) == 0) {
        //lw begin
        Sprite *spt = dynamic_cast<Sprite*>(pNode);
        bool hasPremultipliedAlpha = false;
        auto texture = spt->getTexture();
        if (texture && texture->hasPremultipliedAlpha()) {
            hasPremultipliedAlpha = true;
        } else {
            auto atlas = spt->getTextureAtlas();
            if (atlas) {
                auto texture = atlas->getTexture();
                if (texture && texture->hasPremultipliedAlpha()) {
                    hasPremultipliedAlpha = true;
                }
            }
        }
        if (!hasPremultipliedAlpha) {
            if (pBlendFunc.src == GL_ONE && pBlendFunc.dst == GL_ONE) {
                pBlendFunc = BlendFunc::ADDITIVE;
            } else if (pBlendFunc.src == BlendFunc::ALPHA_PREMULTIPLIED.src && pBlendFunc.dst == BlendFunc::ALPHA_PREMULTIPLIED.dst) {
                pBlendFunc = BlendFunc::ALPHA_NON_PREMULTIPLIED;
            }
        }
        pBlendFunc.src = GL_SRC_ALPHA;
        
        //lw end
        ((Sprite *)pNode)->setBlendFunc(pBlendFunc);
    } else {
        NodeLoader::onHandlePropTypeBlendFunc(pNode, pParent, pPropertyName, pBlendFunc, ccbReader);
    }
}

NS_CC_EXT_END
