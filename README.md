# Data Clustering Contest (Stage II)
The project is made according to the Telegram's contest [instructions](https://contest.com/docs/data_clustering2).

To build this project, open bin/, then run:
~~~
cmake ..
cmake --build .
~~~
To run this project, use:
~~~
tgnews languages <source_dir>
tgnews news <source_dir>
tgnews categories <source_dir>
tgnews threads <source_dir>
tgnews top <source_dir>
tgnews server  <port>
~~~
If you want to run a testing script, use:
~~~
php dc-check.php tgnews all <port> <source_dir>
php dc-check.php tgnews languages <source_dir>
php dc-check.php tgnews news <source_dir>
php dc-check.php tgnews categories <source_dir>
php dc-check.php tgnews threads <source_dir>
php dc-check.php tgnews server <port> <source_dir>

~~~
