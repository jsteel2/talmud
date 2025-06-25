print("<html><body>");
format(print, "<q>%s</q><pre>", celestia());
memrep();
print("</pre>");
httpd_dir(&task{TASK_CWD});
print("</body></html>");