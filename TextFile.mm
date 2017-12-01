//
//  TextFile.m
//  KEngineCore_Mac
//
//  Created by Kelson Hootman on 11/26/17.
//

#import <Foundation/Foundation.h>
#include "TextFile.h"
#include <string>

void KEngineCore::TextFile::LoadFromFile(const std::string& filename, const std::string& extension) {
    NSString * nsFilename = [NSString stringWithUTF8String:filename.c_str()];
    NSString * nsExtension = [NSString stringWithUTF8String:extension.c_str()];
    
    NSString* filePath = [[NSBundle mainBundle] pathForResource:nsFilename
                                                           ofType:nsExtension];
    NSError* error;
    NSString* contents = [NSString stringWithContentsOfFile:filePath
                                                       encoding:NSUTF8StringEncoding error:&error];
    if (!contents) {
        NSLog(@"Error loading text file: %@", error.localizedDescription);
        exit(1);
    }
    mFileContents = [contents UTF8String];
}

const std::string& KEngineCore::TextFile::GetContents() const
{
    return mFileContents;
}
