/* -*- mode: objc -*- */
//
// Project: Workspace
//
// Description: The FileOperation tool's copying functions.
//
// Copyright (C) 2006-2014 Sergii Stoian
// Copyright (C) 2005 Saso Kiselkov
//
// This application is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This application is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public
// License along with this library; if not, write to the Free
// Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA.
//

#import <Foundation/Foundation.h>
#import "../Communicator.h"

BOOL CopyOperation(NSString *sourceDir, NSArray *files, NSString *destDir, OperationType opType);

BOOL CopyFile(NSString *filename, NSString *sourcePrefix, NSString *targetPrefix, BOOL traverseLink,
              OperationType opType);

void DuplicateOperation(NSString *sourceDir, NSArray *files);

BOOL DuplicateSymbolicLink(NSString *sourceFile, NSString *targetFile, NSDictionary *fattrs);

BOOL DuplicateFile(NSString *filename, NSString *sourcePrefix, BOOL traverseLink);
