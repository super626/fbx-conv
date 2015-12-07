/*******************************************************************************
 * Copyright 2011 See AUTHORS file.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************/
/** @author Xoppa */
#ifdef _MSC_VER
#pragma once
#endif //_MSC_VER
#ifndef FBXCONV_READERS_READER_H
#define FBXCONV_READERS_READER_H

#include <stdio.h>

#include "../modeldata/Model.h"

using namespace fbxconv::modeldata;

namespace fbxconv {
namespace readers {

class Reader {
public:
	virtual ~Reader() {}
	virtual bool load(Settings *settings) = 0;
	virtual bool convert(Settings *settings, Model * const &model) = 0;
};
    
    class RawReader : public Reader {
    public:
        RawReader() { _fp = NULL; }
        ~RawReader() {}
        
        std::string readString()
        {
            std::string str;
            int strsize;
            fread(&strsize, sizeof(int), 1, _fp);
            if (strsize > 0)
            {
                char* id = new char[strsize+1];
                fread(id, 1, strsize, _fp);
                id[strsize] = 0;
                str = id;
                delete [] id;
            }
            return str;
        }
        
        virtual bool load(Settings *settings) override
        {
            _fp = fopen(settings->inFile.c_str(), "rb");
            if (_fp == NULL)
                return false;
            return true;
        }
        
        void convertMesh(Model * const &model)
        {
            int meshcount;
            fread(&meshcount, sizeof(int), 1, _fp);
            for(int i = 0; i < meshcount; i++)
            {
                int vertexSize, vertexCount;
                fread(&vertexSize, sizeof(int), 1, _fp);
                fread(&vertexCount, sizeof(int), 1, _fp);
                float * vertex = new float[vertexCount];
                fread(vertex, sizeof(float), vertexCount, _fp);
                std::string meshId = readString();
                int attributes;
                fread(&attributes, sizeof(int), 1, _fp);
                
                Mesh* mesh = new Mesh();
                mesh->id = meshId;
                mesh->vertexSize = vertexSize;
                mesh->vertices = std::vector<float>(vertex, vertex + vertexCount);
                mesh->attributes.value = attributes;
                model->meshes.push_back(mesh);
                
                delete [] vertex;
                
                int meshpartsize;
                fread(&meshpartsize, sizeof(int), 1, _fp);
                for (int j = 0; j < meshpartsize; j++)
                {
                    std::string meshpartId = readString();
                    int indexcount;
                    fread(&indexcount, sizeof(int), 1, _fp);
                    unsigned short* indices = new unsigned short[indexcount];
                    fread(indices, sizeof(unsigned short), indexcount, _fp);
                    unsigned int primitive;
                    fread(&primitive, sizeof(unsigned int), 1, _fp);
                    float aabb[6];
                    fread(aabb, sizeof(float), 6, _fp);
                    
                    MeshPart* meshpart = new MeshPart();
                    meshpart->id = meshpartId;
                    meshpart->indices = std::vector<unsigned short>(indices, indices + indexcount);
                    meshpart->primitiveType = primitive;
                    memcpy(meshpart->aabb, aabb, sizeof(float) * 6);
                    
                    mesh->parts.push_back(meshpart);
                    
                    delete [] indices;
                }
            }
        }
        
        void convertMaterial(Model * const &model)
        {
            int materialCount;
            fread(&materialCount, sizeof(int), 1, _fp);
            for(int i = 0; i < materialCount; i++)
            {
                Material* material = new Material();
                material->id = readString();
                
                int texCount;
                fread(&texCount, sizeof(int), 1, _fp);
                for (int i = 0; i < texCount; i++)
                {
                    Material::Texture* tex = new Material::Texture();
                    tex->id = readString();
                    tex->path = readString();
                    fread(tex->uvTranslation, sizeof(float), 2, _fp);
                    fread(tex->uvScale, sizeof(float), 2, _fp);
                    fread(&tex->usage, sizeof(int), 1, _fp);
                    fread(&tex->wrapModeU, sizeof(int), 1, _fp);
                    fread(&tex->wrapModeV, sizeof(int), 1, _fp);
                    material->textures.push_back(tex);
                }
                model->materials.push_back(material);
            }
        }
        
        void convertNode(Model * const &model)
        {
            Node* node = new Node();
            fread(node->transforms, sizeof(float), 16, _fp);
            node->id = readString();
            int partcount;
            fread(&partcount, sizeof(int), 1, _fp);
            for (int i = 0; i < partcount; i++)
            {
                NodePart* nodepart = new NodePart();
                std::string meshpartId = readString();
                std::string materialId = readString();
                for (auto mesh : model->meshes)
                {
                    for (auto part : mesh->parts)
                    {
                        if (part->id == meshpartId)
                        {
                            nodepart->meshPart = part;
                            break;
                        }
                    }
                    if (nodepart->meshPart != NULL)
                        break;
                }
                for (auto material : model->materials)
                {
                    if (materialId == material->id)
                    {
                        nodepart->material = material;
                        break;
                    }
                }
                int bonecount;
                fread(&bonecount, sizeof(int), 1, _fp);
                for (int k = 0; k < bonecount; k++)
                {
                    //to do
                }
                node->parts.push_back(nodepart);
            }
            model->nodes.push_back(node);
            int childcount;
            fread(&childcount, sizeof(int), 1, _fp);
            for(int i = 0; i < childcount; i++)
            {
                convertNode(model);
            }
        }
        
        virtual bool convert(Settings *settings, Model * const &model) override
        {
            //convert mesh
            convertMesh(model);
            
            //conver material
            convertMaterial(model);
            
            int nodecount;
            fread(&nodecount, sizeof(int), 1, _fp);
            for (int i = 0; i < nodecount; i++)
                convertNode(model);
            
            fclose(_fp);
            
            return true;
        }
        
    protected:
        FILE* _fp;
    };

} // readers
} // fbxconv

#endif //FBXCONV_READERS_READER_H