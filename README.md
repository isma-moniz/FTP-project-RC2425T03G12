# FTP-project-RC2425T03G12

## Description
A simple FTP client implementation, developed in the context of RC's Project 2 @ FEUP.

Handles errors, has anonymous and authenticated access, and can download files of all sizes tested so far (up to ~380MB was tested).

Also presents detailed server responses to the user, a feature we thought was lacking in other implementations published online.

## Usage
1. Adjust default anonymous credentials if needed in *include/FTP.h*
2. ```make clean && make download```
3. ```ftp://[<user>:<password>@]<host>/<url-path>```
