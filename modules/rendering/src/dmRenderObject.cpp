#include <v4r/rendering/dmRenderObject.h>
#include <iostream>
#include <glm/glm.hpp>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>

namespace v4r
{

DepthmapRendererModel::DepthmapRendererModel(std::string file)
{
    //set buffer references to zero:

    VBO=0;
    IBO=0;

    vertexCount=0;
    indexCount=0;
    geometry=false;

    color=false;
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile( file,
            aiProcess_CalcTangentSpace       |
            aiProcess_Triangulate            |
            aiProcess_JoinIdenticalVertices  |
            aiProcess_SortByPType);

    if( !scene)
    {
      std::cout << importer.GetErrorString() << std::endl;
      return;
    }

    geometry=true;


    glm::dvec3 mean(0,0,0);
    //first count how many geometry we need + find out maxima and average...
    if(scene->HasMeshes()){
        for(int i=0;i<scene->mNumMeshes;i++){

            vertexCount+=scene->mMeshes[i]->mNumVertices;
            //TODO: faces sind einzeln unterteilt
            for(int j=0;j<scene->mMeshes[i]->mNumFaces;j++){
                indexCount+=scene->mMeshes[i]->mFaces[i].mNumIndices;
            }
            std::cout << "Mesh nr:" << i << " faces:"<< scene->mMeshes[i]->mNumFaces << "  Vertices:"<< scene->mMeshes[i]->mNumVertices << std::endl;
            for(int j=0;j<scene->mMeshes[i]->mNumVertices;j++){
                //print out vertex data:
                glm::vec3 vertex(scene->mMeshes[i]->mVertices[j].x,scene->mMeshes[i]->mVertices[j].y,scene->mMeshes[i]->mVertices[j].z);
                //std::cout << " vertex:" <<vertex.x << " " << vertex.y << " " << vertex.z << std::endl;//Debug
                mean+=glm::dvec3(vertex.x,vertex.y,vertex.z);
                color=color||scene->mMeshes[i]->HasVertexColors(0);
            }
        }
    }
    mean=mean/((double)vertexCount);
    //last: create memory for geometry
    indices=new unsigned int[indexCount];
    vertices=new Vertex[vertexCount];
    double maxDistToCenter=0.0;

    unsigned int k=0;
    unsigned int l=0;
    unsigned int m=0;
    for(int i=0;i<scene->mNumMeshes;i++){
        for(int j=0;j<scene->mMeshes[i]->mNumVertices;j++){
            Vertex v;
            v.pos=glm::vec3(scene->mMeshes[i]->mVertices[j].x,
                            scene->mMeshes[i]->mVertices[j].y,
                            scene->mMeshes[i]->mVertices[j].z)-glm::vec3(mean);
            if(scene->mMeshes[i]->HasVertexColors(0)){
                aiColor4D* colors= scene->mMeshes[i]->mColors[0];
                v.rgba=glm::u8vec4(colors[j].r*255.0f,colors[j].g*255.0f,colors[j].b*255.0f,colors[j].a*255.0f);
                //v.rgba=glm::u8vec4(255,255,255,255);//debug
            }else{
                v.rgba=glm::u8vec4();
            }
            vertices[k]=v;
            float distToCenter=glm::length(vertices[k].pos);
            if(distToCenter>maxDistToCenter){
                maxDistToCenter=distToCenter;
            }
            k++;
        }

        for(int j=0;j<scene->mMeshes[i]->mNumFaces;j++){
            for(int n=0;n<scene->mMeshes[i]->mFaces[j].mNumIndices;n++){

                indices[m]=scene->mMeshes[i]->mFaces[j].mIndices[n]+l;
                m++;
            }
        }
        l+=scene->mMeshes[i]->mNumVertices;
    }



    offset=-glm::vec3(mean);

    scale=1.0f/(float)maxDistToCenter;

    //scale it:
    for(int i=0;i<vertexCount;i++){
        vertices[i].pos=vertices[i].pos*(1.0f/(float)maxDistToCenter);
        //std::cout << " vertex:" <<vertices[i].x << " " << vertices[i].y << " " << vertices[i].z << std::endl;//Debug
    }




}

DepthmapRendererModel::~DepthmapRendererModel()
{
    //well! not much to destroy^^
    if(VBO!=0){
        //except for occational buffers
        glDeleteBuffers(1,&VBO);
        glDeleteBuffers(1,&IBO);

        //delete VAO

    }
    delete[] vertices;
    delete[] indices;
}

void DepthmapRendererModel::loadToGPU()
{

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*vertexCount, vertices, GL_STATIC_DRAW);


    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*indexCount, indices, GL_STATIC_DRAW);


    //Make sure the shader is bound beforehand: (set the attribute location)


}


unsigned int DepthmapRendererModel::getIndexCount()
{
    return indexCount;
}

float DepthmapRendererModel::getScale(){
    return scale;
}

bool DepthmapRendererModel::hasColor()
{
    return color;
}

Eigen::Vector3f DepthmapRendererModel::getOffset(){
    return Eigen::Vector3f(offset.x,offset.y,offset.z);
}

}
