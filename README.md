# 2022-group-15

## Name

Project: DIT638 Cyber-Physical Systems and Systems-of-Systems

## Description

The purpose of this project is to develop software features for self-driving vehicles in a data-driven way.

A more detailed description will be added during the progression of the project.

### Setting up the project

- Note: You need to use SSH key to access Gitlab in order to be able to clone this repo. Please follow the [instruction](https://docs.gitlab.com/ee/user/ssh.html) to add your ssh key to your GitLab account

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

Screen shots and videos of the final product will be added at the end of the project.

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
We will have a git-based workflow using branches. We will use topic branches which are short-lived branches that you create and use for a single particular feature or related work. Branches should be deleted after the feature is merged.

#### Commits
To ensure that our code is traceable we will strive to have small commits that focus on one single responsibility. The commits should be linked to issues in order to ensure traceability. 

##### Commit message structure
We will follow the Conventional Commit structure for our commit messages. 

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
We strive to create clear merge requests to simplify code reviews from the team. In order to achieve this, we:

- Follow the MR template
- Strive to keep the MR small, i.e. strive for < 100 lines of code for each merge request if possible. 
- Link to relevant issues and milestones

#### Code reviews
As a team, we strive to always ensure that our code base is of high quality. We help eachother to achive this byt conducting code reviews. The code review takes place after a merge request has been done. During a code review, pay attention to these aspects:

1. Check if the code does what it is supposed to do. Does it meet the requirements? Will the stakeholders be happy with what we deliver?
2. How does the code accomplish the requirements? Is it clean and readable? Does it follow best practices?
3. Is the code able to accommodate potential changes in the future?
4. Look at the potential for improvements. Is there a simpler way to achieve the same goals?
5. Is the code well-documented? Are the commit messages clear and follow the standard?
6. Keep a friendly tone and create a kind environment. Giving compliments is always appreciated. 

### Adding new features
Before adding new features, we decide as a team on what to implement during Sprint planning meetings. Features are distributed during the meeting. When assignmed a feature, we:
- Create a new issue on GitLab
- The issue should contain what requirements it is linked to, relevant labels and acceptance criteria
- When the implementation is done, create a merge request

### Fix unexpected behaviours
If unexpected behaviours are found, we appreciate if the issues are acknowledges and fixed.
- Create an issue to report the behavior
- If needed, discuss priority of the issue within the team
- Explain the unexpected behavior and what the expected behavior is supposed to be
- Add relevant labels
- Assign a developer to the issue

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
