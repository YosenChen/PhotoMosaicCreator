#ifndef IMAGE_FILE_MANAGER_H
#define IMAGE_FILE_MANAGER_H

#include <dirent.h>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <unordered_set>
#include <assert.h>     /* assert */

class ImageFileManager
{
public:
    bool FindFile(std::string folderPath,
                  std::unordered_set<std::string> fileExt)
    {
        m_dir = opendir(folderPath.c_str());
        if (!m_dir)
        {
            perror("");
            return false;
        }
        m_folderPath = folderPath;
        m_fileExt = fileExt;
        return true;
    }

    std::string GetFileName()
    {
        assert(m_ent != NULL);
        return m_ent->d_name;
    }

    bool FindNextFile()
    {
        while ((m_ent = readdir(m_dir)) != NULL)
        {
            std::string filename{m_ent->d_name};
            if (p_IsMatchedFileExt(filename)) return true;
        }
        return false;
    }

    void Close()
    {
        closedir(m_dir);
    }
        
private:

    bool p_IsMatchedFileExt(const std::string& filename)
    {
        // first find the file extension
        int backIdx;
        for (backIdx = filename.size()-1; backIdx >= 0; backIdx--)
        {
            if (filename[backIdx] == '.') break;
        }
        if (backIdx == 0) return false; // no file extension

        auto ext = filename.substr(backIdx);
        return (m_fileExt.find(ext) !=  m_fileExt.end());
    }

    std::string m_folderPath{};
    std::unordered_set<std::string> m_fileExt{};
    DIR *m_dir{NULL};
    struct dirent *m_ent{NULL};
};

#endif // IMAGE_FILE_MANAGER_H
