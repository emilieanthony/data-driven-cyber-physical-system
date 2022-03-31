# 2022-group-15

## Name

Data-driven software for self-driving vehicles

## Description

Today’s vehicles are equipped with many embedded systems to provide comfort and safety functions to the driver, passengers, and other traffic participants. The purpose of this project is to develop software features for self-driving vehicles in a data-driven way. The project is primarily working with data from scaled vehicles develop software features. 

More informtion will be added.

### Setting up the project

- Note: You need to use SSH key to access Gitlab in order to be able to clone this repo. Please follow the [instruction](https://docs.gitlab.com/ee/user/ssh.html) to add your ssh key to your gitlab account

#### Tools you need

- docker: [Installation](https://docs.docker.com/engine/install/)
- docker-compose version: 2.2.3: [Installation](https://docs.docker.com/compose/install/)
- git: [Installation](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git)
- an IDE (we recommand to use Vscode)

After you installed all required tools, you could use the following commands to clone this repo and set up at your computer.

~~~
mkdir your-folder-name
cd your-folder-name
git clone git@git.chalmers.se:courses/dit638/students/2022-group-15.git
cd 2022-group-15
docker-compose up //for the first time
docker-compose start //for the rest
~~~

Then in a new terminal client, enter following commands to connect to the bash of the docker container.

~~~
docker images //to show all existing images
docker exec -it dit638-g15 bash
~~~

From there you could start to work with the project.

To work with example project, you can use follow commands inside docker image.

~~~
cd /example\ project\build
cmake ..
make
make test
./helloworld 4
~~~

It should output:

~~~
firstname, lastname;4 is a prime number? 0
~~~

You can use `exit` to leave the docker container bash.

To remove the docker container:

~~~
docker-compose down
~~~


## Visuals

Screen shots and videos will be added at the end of the project.

## Contributing

### Way-of-working

#### Definition of Done
To be able to ensure high quality of the system, we strive to meet the definition of done (DoD) during development. We consider features on a delivery team level as done when the product meets the requirements of the system. The requirements are set up by the team and in close collaboration with stakeholders. 
 
 DoD for the project:
- Code is reviewed
- Acceptance critera met
- Automated/manual (unit) tests pass
- Integrated to a clean build
- Non-functional requirements met
- Meets compliance requirements
- Functionality documented in necessary documentation
- Stakeholders accept the feature


#### Protected main
Our main branch is protected. This means that there are certain requirements to be met before a collaborator can push changes to the main branch.

- We require an approved merge requests before being able to push
- The pipeline should pass
- The team member who initiates the merge request is the one responsible to merge it into main

#### Branching

#### Commits
To ensure that our code is traceable we will strive to have small commits that focus on a single responsibility. 
The commits should be linked to issues.


##### Commit message structure
We will follow the Conventional Commit structure for our commit messages. 
We aim to link each commit to an open issue, using keywords. 

Limit the subject line up to 50 characters. 

Do not end the sentence with a period, plus use 

```
<type>[optional scope]: <description>
```
The commit body should consist of a maximum of 72 characters per line.  
Explaining what was changed and the reason.
```
[optional body]
[optional footer(s)]
```

Example: 
```
fix: minor typos in the code
Typos were fixed in the code that was leading to problems
in the system. Resolves #5. 
```
 


#### Merge Requests

#### Code reviews

### Adding new features

### Fix unexpected behaviours


## Authors and acknowledgment

### Contributors:
- Astrid Berntsson
- Emilie Anthony
- Olga Ratushniak
- Renyuan Huang
- Samandar Hashimi

## License

See [LICENSE](https://git.chalmers.se/courses/dit638/students/2022-group-15/-/blob/main/LICENSE)

## Project status

This project is part of the course DIT638 that runs from March 2022 to May 2022. 
