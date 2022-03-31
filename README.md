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

State if you are open to contributions and what your requirements are for accepting them.

For people who want to make changes to your project, it's helpful to have some documentation on how to get started. Perhaps there is a script that they should run or some environment variables that they need to set. Make these steps explicit. These instructions could also be useful to your future self.

You can also document commands to lint the code or run tests. These steps help to ensure high code quality and reduce the likelihood that the changes inadvertently break something. Having instructions for running tests is especially helpful if it requires external setup, such as starting a Selenium server for testing in a browser.

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
