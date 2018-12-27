import { Component, OnInit } from '@angular/core';

@Component({
  selector: 'app-messages',
  templateUrl: './messages.component.html',
  styleUrls: ['./messages.component.scss']
})
export class MessagesComponent implements OnInit {

  messages = [
    {
      is_new: true,
      address: '@bitmap',
      message: 'No more miners for you!'
    },
    {
      is_new: false,
      address: 'Hjkwey36gHasdhkajshd4bxnb5mcvowyefb2633FdsFGGWbb',
      message: 'Hey! What’s with our BBR deal?'
    },
    {
      is_new: false,
      address: '@john',
      message: 'I’m coming!'
    }
  ];

  constructor() {}

  ngOnInit() {}


}
